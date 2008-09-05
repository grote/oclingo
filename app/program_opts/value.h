//
//  ProgramOptions
//  (C) Copyright Benjamin Kaufmann, 2004 - 2005
//	Permission to copy, use, modify, sell and distribute this software is 
//	granted provided this copyright notice appears in all copies. 
//	This software is provided "as is" without express or implied warranty, 
//	and with no claim as to its suitability for any purpose.
//
//  ProgramOptions is a scaled-down version of boost::program_options
//  see: http://boost-sandbox.sourceforge.net/program_options/html/
// 
#ifndef PROGRAM_OPTIONS_VALUE_H_INCLUDED
#define PROGRAM_OPTIONS_VALUE_H_INCLUDED
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
#endif
#include "program_options.h"
#include "value_base.h"
#include "errors.h"
#include <typeinfo>
#include <string>
#include <memory>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>
namespace ProgramOptions {

//! typed value of an option
template <class T>
class Value : public ValueBase
{
public:
	typedef bool (*CustomParser)(const std::string&, T&, T*);
	/*!
	* \param storeTo where to store the value once it is known.
	* \note if storeTo is 0 Value allocates new memory for the value.
	*/
	Value(T* storeTo = 0)
		: value_(storeTo)
		, defaultValue_(0)
		, parser_(0)
		, deleteValue_(storeTo == 0)
		, implicit_(false)
		, defaulted_(false)
		, composing_(false)
		, hasValue_(false)
	{}

	Value(const Value<T>& other)
		: value_(other.value_ ? new T(*other.value_) : 0)
		, defaultValue_(other.defaultValue_ ? new T(*other.defaultValue_) : 0)
		, parser_(other.parser_)
		, deleteValue_(true)
		, implicit_(other.implicit_)
		, defaulted_(other.defaulted_)
		, composing_(other.composing_)
		, hasValue_(other.hasValue_)
	{}

	Value& operator=(const Value<T>& other)
	{
		if (this != &other)
		{
			std::auto_ptr<T> newVal(other.value_ ? new T(*other.value_) : 0);
			std::auto_ptr<T> newDefVal(other.defaultValue_ ? new T(*other.defaultValue_) : 0);
			if (deleteValue_)
				delete value_;
			delete defaultValue_;
			value_ = newVal.release();
			deleteValue_ = true;
			defaultValue_ = newDefVal.release();
			parser_ = other.parser_;
			implicit_ = other.implicit_;
			defaulted_ = other.defaulted_;
			composing_ = other.composing_;
			hasValue_ = other.hasValue_;
		}
		return *this;
	}
	~Value()
	{
		if (deleteValue_)
			delete value_;
		delete defaultValue_;
	}

	bool hasValue()			const		{return hasValue_; }
	bool isImplicit()		const		{return implicit_;}
	bool isComposing()	const		{return composing_;}
	bool isDefaulted()	const		{return defaulted_;}

	bool parse(const std::string&);

	const T& value() const
	{
		if (!value_)
			throw BadValue("no value");
		return *value_;
	}
	T& value()
	{
		if (!value_)
			throw BadValue("no value");
		return *value_;
	}

	//! sets a defaultValue for this value
	Value<T>* defaultValue(const T& t)
	{
		T* nd = new T(t);
		delete defaultValue_;
		defaultValue_ = nd;
		return this;

	}

	bool applyDefault()
	{
		if (defaultValue_)
		{
			if (value_)
			{
				*value_ = *defaultValue_;
			}
			else
			{
				value_ = new T(*defaultValue_);
				deleteValue_ = true;
			}
			defaulted_ = true;
			return true;
		}
		return false;
	}

	//! marks this value as implicit
	Value<T>* setImplicit()
	{
		implicit_ = true;
		return this;
	}

	//! marks this value as composing
	Value<T>* setComposing()
	{
		composing_ = true;
		return this;
	}

	Value<T>* parser(CustomParser pf) { parser_ = pf; return this; }	
private:
	T* value_;
	T* defaultValue_;
	CustomParser parser_;
	mutable bool deleteValue_;
	bool implicit_;
	bool defaulted_;
	bool composing_;
	bool hasValue_;
};

template <class T>
bool parseValue(const std::string& s, T& t, double)
{
	std::stringstream str;
	str << s;
	if ( (str>>t) ) {
		std::char_traits<char>::int_type c;
		for (c = str.get(); std::isspace(c); c = str.get());
		return c == std::char_traits<char>::eof();
	}
	return false;
}

bool parseValue(const std::string& s, bool& b, int);
bool parseValue(const std::string& s, std::string& r, int);

template <class T>
bool parseValue(const std::string& s, std::vector<T>& result, int)
{
	std::stringstream str(s);
	for (std::string item; std::getline(str, item, ',');) {
		T temp;
		if (!parseValue(item, temp, 1)) {
			return false;
		}
		result.push_back(temp);
	}
	return str.eof();
}

template <class T, class U>
bool parseValue(const std::string& s, std::pair<T, U>& result, int) {
	std::string copy(s);
	for (std::string::size_type i = 0; i < copy.length(); ++i) {
		if (copy[i] == ',') copy[i] = ';';
	}
	std::stringstream str(copy);
	if ( !(str >> result.first) ) {return false; }
	str >> std::skipws;
	char c;
	if (str.peek() == ';' && !(str >> c >> result.second)) {
		return false;
	}
	return str.eof();
}

template <class T>
bool Value<T>::parse(const std::string& s)
{
	if (!value_)
	{
		value_ = new T();
		deleteValue_ = true;
	}
	bool ret = parser_ 
		? parser_(s, *value_, defaultValue_)
		: parseValue(s, *value_, 1);
	if (ret) {
		defaulted_ = false;
		hasValue_ = true;
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// value creation functions
///////////////////////////////////////////////////////////////////////////////
//! creates a new Value-object for values of type T
template <class T>
Value<T>* value(T* v = 0)
{
	return new Value<T>(v);
}

//! creates a Value-object for options that are bool-switches like --help or --version
/*!
* \note same as value<bool>()->setImplicit()
*/
Value<bool>* bool_switch(bool* b = 0);

///////////////////////////////////////////////////////////////////////////////
// option to value functions
///////////////////////////////////////////////////////////////////////////////

//! down_cast for Value-Objects.
/*!
* \throw BadValue if opt is not of type Value<T>
*/
template <class T>
const T& value_cast(const ValueBase& opt, T* = 0)
{
	if (const Value<T>* p = dynamic_cast<const Value<T>*>(&opt))
		return p->value();
	std::string err = "value is not an ";
	err += typeid(T).name();
	throw BadValue(err.c_str());
}

template <class T>
T& value_cast(ValueBase& opt, T* = 0)
{
	if (Value<T>* p = dynamic_cast<Value<T>*>(&opt))
		return p->value();
	std::string err = "value is not an ";
	err += typeid(T).name();
	throw BadValue(err.c_str());
}

template <class T, class U>
const T& option_as(const U& container, const char* name, T* = 0)
{
	try {
		return ProgramOptions::value_cast<T>(container[name]);
	}
	catch(const BadValue& v)
	{
		std::string msg = "Option ";
		msg += name;
		msg += ": ";
		msg += v.what();
		throw BadValue(msg);
	}
}

}
#endif
