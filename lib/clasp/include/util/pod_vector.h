// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef BK_LIB_POD_VECTOR_H_INCLUDED
#define BK_LIB_POD_VECTOR_H_INCLUDED
#include <iterator>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <stdexcept>
namespace bk_lib { namespace detail {
	
	struct MemCpyRange {
		MemCpyRange(const void* start, std::size_t size) : start_(start), size_(size) {}
		void operator()(void* dest) const {
			std::memcpy(dest, start_, size_);
		}
		const void* start_;
		std::size_t size_;
	};
	
	template <class T>
	void fill(T* first, T* last, const T& x) {
		assert(first <= last);
		switch ((last - first) & 7u)
		{
		case 0:
				while (first != last)
				{
						new(first) T(x); ++first;
		case 7: new(first) T(x); ++first;
		case 6: new(first) T(x); ++first;
		case 5: new(first) T(x); ++first;
		case 4: new(first) T(x); ++first;
		case 3: new(first) T(x); ++first;
		case 2: new(first) T(x); ++first;
		case 1: new(first) T(x); ++first;
						assert(first <= last);
				}
		}
	}

	template <class Iter, class T>
	void copy(Iter first, Iter last, std::size_t s, T* out) {
		switch (s & 7u)
		{
		case 0:
				while (first != last)
				{
						new(out++) T(*first); ++first;
		case 7: new(out++) T(*first); ++first;
		case 6: new(out++) T(*first); ++first;
		case 5: new(out++) T(*first); ++first;
		case 4: new(out++) T(*first); ++first;
		case 3: new(out++) T(*first); ++first;
		case 2: new(out++) T(*first); ++first;
		case 1: new(out++) T(*first); ++first;
				}
		}
	}

	template <class T>
	struct Fill {
		Fill(const T& val, std::size_t n) : val_(val), n_(n) {}
		void operator()(T* first) const {
			detail::fill(first, first + n_, val_);
		}
		const T& val_;
		std::size_t n_;
	};

	template <class Iter>
	struct Copy {
		Copy(Iter first, Iter last, std::size_t n) : first_(first), last_(last), n_(n) {}
		template <class T>
		void operator()(T* out) const {
			detail::copy(first_, last_, n_, out);
		}
		Iter first_;
		Iter last_;
		std::size_t n_;
	};
	
	template <int i> struct Int2Type {};
	typedef char yes_type;
	typedef char (&no_type)[2];
	template <class T>
	struct IterType {
		static yes_type isPtr(const volatile void*);
		static no_type isPtr(...);
		static yes_type isLong(long);
		static no_type  isLong(...);
		static T& makeT();
		enum { ptr = sizeof(isPtr(makeT())) == sizeof(yes_type) };
		enum { num = sizeof(isLong(makeT())) == sizeof(yes_type) }; 
		enum { value = ptr ? 1 : num ? 2 : 0 };
	};

} // end namespace bk_lib::detail

//! A std::vector-replacement for POD-Types. 
/*!
 * \pre T is a POD-Type 
 * \see http://www.comeaucomputing.com/techtalk/#pod for a description of POD-Types.
 */
template <class T, class Allocator = std::allocator<T> >
class pod_vector {
public:
	// types:
	typedef          pod_vector<T, Allocator>         this_type;//not standard
	typedef typename Allocator::reference             reference;
	typedef typename Allocator::const_reference       const_reference;
	typedef typename Allocator::pointer               iterator;
	typedef typename Allocator::const_pointer         const_iterator;
	typedef typename Allocator::size_type             size_type;
	typedef typename Allocator::difference_type       difference_type;
	typedef          T                                value_type;
	typedef          Allocator                        allocator_type;
	typedef typename Allocator::pointer               pointer;
	typedef typename Allocator::const_pointer         const_pointer;
	typedef std::reverse_iterator<iterator>           reverse_iterator;
	typedef std::reverse_iterator<const_iterator>     const_reverse_iterator;

	// ctors
	//! constructs an empty pod_vector.
	/*! 
	 * \post size() == capacity() == 0
	 */
	pod_vector() {
		init_empty();
	}
	
	//! constructs an empty pod_vector that uses a copy of a for memory allocations.
	/*!
	 * \post size() == capacity() == 0
	 */
	explicit pod_vector(const allocator_type& a) : ebo_(a) {
		init_empty();
	}

	//! constructs a pod_vector containing n copies of value.
	/*!
	 * \post size() == n
	 */
	explicit pod_vector(size_type n, const T& value = T(), const Allocator& a = Allocator()) 
		: ebo_(a) {
		n != 0 ? init(n, value) : init_empty();
	}

	//! constructs a pod_vector equal to the range [first, last).
	/*!
	 * \post size() = distance between first and last.
	 */
	template <class Iter>
	pod_vector(Iter first, Iter last, const Allocator& a = Allocator()) 
		: ebo_(a) {                
		init_empty();
		insert(end(), first, last);
	}
	
	//! creates a copy of other
	/*!
	 * \post size() == other.size() && capacity() >= other.capacity()
	 */
	pod_vector(const pod_vector& other) {
		init_empty();
		append(other.begin(), other.end());
	}

	
	pod_vector& operator=(pod_vector other) {
		other.swap(*this);
		return *this;
	}

	//! frees all memory allocated by this pod_vector.
	/*!
	 * \note Won't call any destructors, because PODs don't have those.
	 */
	~pod_vector() {
		if (capacity() != 0) {
			ebo_.deallocate(ebo_.first_, capacity());
		}
	}

	/** @name inspectors
	 * inspector-functions
	 */
	//@{
	
	//! returns the number of elements currently stored in this pod_vector.
	size_type size() const    { return last_ - ebo_.first_; }
	
	//! size of the largest possible pod_vector
	size_type max_size() const { return ebo_.max_size(); }
	
	//! returns the total number of elements this pod_vector can hold without requiring reallocation.
	size_type capacity() const  { return eos_ - ebo_.first_; }
	
	//! returns size() == 0
	bool empty() const { return ebo_.first_ == last_;  }

	
	iterator begin() { return ebo_.first_; }
	const_iterator begin() const { return ebo_.first_;}

	iterator end() { return last_; }
	const_iterator end() const { return last_;}

	reverse_iterator rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	//! returns a copy of the allocator used by this pod_vector
	allocator_type get_allocator() const { return ebo_; }
	
	//@}
	/** @name elemacc
	 * element access
	 */
	//@{
	
	//! returns a reference to the element at position n
	/*!
	 * \pre n < size()
	 */
	reference operator[](size_type n) {
		assert(n < size());
		return ebo_.first_[n];
	}
	
	//! returns a reference-to-const to the element at position n
	/*!
	 * \pre n < size()
	 */
	const_reference operator[](size_type n) const {
		assert(n < size());
		return ebo_.first_[n];
	}
	
	//! same as operator[] but throws std::range_error if pre-condition is not met.
	const_reference at(size_type n) const {
		if (n >= size()) throw std::range_error("pod_vector::at");
		return ebo_.first_[n];
	}
	//! same as operator[] but throws std::range_error if pre-condition is not met.
	reference at(size_type n) {
		if (n >= size()) throw std::range_error("pod_vector::at");
		return ebo_.first_[n];
	}
	
	//! equivalent to *begin()
	reference front() {
		assert(!empty());
		return *ebo_.first_;
	}
	
	//! equivalent to *begin()
	const_reference front() const {
		assert(!empty());
		return *ebo_.first_;
	}
	
	//! equivalent to *--end()
	reference back() {
		assert(!empty());
		return last_[-1];
	}
	
	//! equivalent to *--end()
	const_reference back() const {
		assert(!empty());
		return last_[-1];
	}
	
	//@}
	/** @name mutators
	 * mutator functions
	 */
	//@{
	
	//! erases all elements in the range [begin(), end)
	/*!
	 * \post size() == 0
	 * \note clearing is a constant-time operation because no destructors have to be called
	 * for POD-Types.
	 */
	void clear() {
		last_ = ebo_.first_;
	}

	
	void assign(size_type n, const T& val) {
		clear();
		insert(end(), n, val);
	}

	template <class Iter>
	void assign(Iter first, Iter last) {
		clear();
		insert(end(), first, last);
	}
	
	//! erases the element pointed to by pos.
	/*!
	 * \pre pos != end() && !empty()
	 * \return an iterator pointing to the element following pos (before that element was erased)
	 * of end() if no such element exists.
	 *
	 * \note since POD-Types don't have destructors erasing an element is a noop. 
	 *  Elements after pos are moved using memmove instead of copied by calling an 
	 *  assignment-operator.
	 * \note invalidates all iterators and references referring to elements after pos.
	 */
	iterator erase(iterator pos) {
		assert(!empty() && pos != end());
		erase(pos, pos + 1);
		return pos;
	}

	//! erases the elements in the raneg [first, last)
	/*!
	 * \pre [first, last) must be a valid range.
	 */
	iterator erase(iterator first, iterator last) {
		if (end() - last > 0) {
			std::memmove(first, last, (end() - last) * sizeof(T));
		}
		last_ -= last - first;
		return first;
	}

	//! adjusts the size of this pod_vector to ns.
	/*!
	 * resize is equivalent to:
	 * if ns > size insert(end(), ns - size(), val)
	 * if ns < size erase(begin() + ns, end())
	 *
	 * \post size() == ns
	 */
	void resize(size_type ns, const T& val = T()) {
		const size_type old = size();
		if (old >= ns) {
			last_ = ebo_.first_ + ns;
		}
		else {
			grow(ns);
			detail::fill(last_, last_ + (ns - old), val);
			last_ = ebo_.first_ + ns;
		}
		assert(size() == ns);
	}

	//! reallocates storage if necessary but never changes the size() of this pod_vector.
	/*!
	 * \note if n is <= capacity() reserve is a noop. Otherwise a reallocation takes place
	 * and capacity() >= n after reserve returned.
	 * \note reallocation invalidates all references, pointers and iterators referring to
	 * elements in this pod_vectror.
	 *
	 * \note when reallocation occurs elements are copied from the old storage using memcpy.
	 * This is safe since POD-Types don't have copy-ctors.
	 */
	void reserve(size_type n) {
		const size_type s = size(), c = capacity();
		if (c >= n) return;
		T* temp = ebo_.first_;
		ebo_.first_ = ebo_.allocate(n);
		if (c) {
			std::memcpy(ebo_.first_, temp, s * sizeof(T));
			ebo_.deallocate(temp, c);
		}
		last_ = ebo_.first_ + s;
		eos_ = ebo_.first_ + n;
		assert(capacity() == n);
		assert(size() == s);
	}

	void swap(pod_vector& other) {
		std::swap(ebo_.first_, other.ebo_.first_);
		std::swap(last_, other.last_);
		std::swap(eos_, other.eos_);
	}

	//! equivalent to insert(end(), x);
	void push_back(const T& x) {
		if (size() == capacity())  {
			// x may be in sequence - copy it
			T temp(x);
			grow(size()+1);
			new (last_) T(temp);
		}
		else {
			new(last_) T(x);
		}
		++last_;
	}
	
	//! equivalent to erase(--end());
	/*!
	 * \pre !empty()
	 */
	void pop_back() {
		assert(!empty());
		--last_;
	}

	//! inserts a copy of val before pos.
	/*!
	 * \pre pos is a valid iterator.
	 * \return an iterator pointing to the copy of val that was inserted.
	 * \note if size() + 1 > capacity() reallocation occurs. Otherwise iterators and
	 * references referring to elements before pos remain valid.
	 * 
	 */
	iterator insert(iterator pos, const T& val) {
		return insert(pos, (size_type)1, val);
	}

	//! inserts n copies of val before pos.
	/*!
	 * \pre pos is a valid iterator.
	 */
	iterator insert(iterator pos, size_type n, const T& val) {
		return insert_impl(pos, n, detail::Fill<T>(val, n));
	}

	//! inserts copies of elements in the range [first, last) before pos.
	/*!
	 * \pre first and last are not iterators into this pod_vector.
	 * \pre pos is a valid iterator.
	 * \note if first and last are pointers, memcpy is used to insert the elements
	 * in the range [first, last) into this container.
	 * 
	 */
	template <class Iter>
	void insert(iterator pos, Iter first, Iter last) {
		insert_select(pos, first, last, detail::Int2Type<detail::IterType<Iter>::value>());
	}

	
	/** @name nonstd
	 * Non-standard interface 
	 */
	//@{
	//! optimized version of insert(end(), first, last)
	void append(const_iterator first, const_iterator last) {
		assert(first <= last);
		size_type dist = (last - first);
		reserve( size() +  dist );
		std::memcpy(end(), first, dist * sizeof(T));
		last_ += dist;
	}

	//! adjusts the size of this pod_vector to ns.
	/*!
	 * In contrast to pod_vector::resize this function does not
	 * initializes new elements in case ns > size().
	 * This reflects the behaviour of built-in arrays of pod-types.
	 * \note 
	 *  Any access to an unitialized element is illegal unless it is accessed
	 *  in order to assign a new value.
	 */
	void resize_no_init(size_type ns) {
		const size_type old = size();
		if (old >= ns) {
			last_ = ebo_.first_ + ns;
		}
		else {
			reserve(ns);
			last_ = ebo_.first_ + ns;
		}
		assert(size() == ns);
	}
	//@}
private:
	void init_empty() {
		ebo_.first_ = 0;
		last_ = 0;
		eos_ = 0;
	}
	void init(size_type s, const T& value) {
		ebo_.first_ = ebo_.allocate(s);
		detail::fill(ebo_.first_, ebo_.first_ + s, value);
		last_ = ebo_.first_ + s;
		eos_ = last_;
	}
	
	void grow(size_type minCap) {
		if (capacity() > minCap) return;
		minCap = minCap < 4 ? 4 : minCap;
		size_type cap = static_cast<size_type>(capacity() * 1.5);
		reserve(cap < minCap ? minCap : cap);
	}

	template <class Iter>
	void insert_impl(iterator pos, Iter first, Iter last, std::forward_iterator_tag) {
		size_type n = std::distance(first, last);
		insert_impl(pos, n, detail::Copy<Iter>(first, last, n));
	}
	template <class Iter>
	void insert_impl(iterator pos, Iter first, Iter last, std::input_iterator_tag) {
		for (; first != last; ++first, ++pos)
			pos = insert(pos, *first);
	}
	
	template <class Iter>
	void insert_select(iterator pos, Iter first, Iter last, detail::Int2Type<0>) {
		insert_impl(pos, first, last, std::iterator_traits<Iter>::iterator_category());
	}
	
	// ptr
	template <class Iter>
	void insert_select(iterator pos, Iter first, Iter last, detail::Int2Type<1>) {
		insert_impl(pos, last - first, detail::MemCpyRange(first, (last - first)*sizeof(T)));
	}
	// num
	template <class Iter>
	void insert_select(iterator pos, Iter first, Iter last, detail::Int2Type<2>) {
		insert(pos, (size_type)first, last);
	}


	void move_right(iterator pos, size_type n) {
		assert( (pos || n == 0) && (eos_ - pos) >= (int)n);
		std::memmove(pos + n, pos, (end() - pos) * sizeof(T));
	}

	template <class P>
	iterator insert_impl(iterator pos, size_type n, P pred) {
		if (size() + n <= capacity()) {
			move_right(pos, n);
			pred(pos);
		}
		else if (capacity() == 0) {
			assert(pos == 0);
			reserve(n);
			pred(ebo_.first_);
			pos = ebo_.first_;
		}
		else {
			const size_type off = pos == 0 ? 0 : pos - begin();
			this_type temp;
			temp.reserve(size() + n);
			std::memcpy(temp.ebo_.first_, ebo_.first_, off * sizeof(T));
			temp.last_ += off;
			pos = temp.ebo_.first_ + off;
			pred(pos);
			iterator remStart = ebo_.first_ + off;
			size_type copyRem = (end() - remStart);
			temp.last_ += copyRem;
			std::memcpy(pos + n, remStart, copyRem * sizeof(T));  
			swap(temp);
		}
		last_ += n;
		return pos;
	}
	struct ebo : public Allocator {
		T* first_;
		ebo() {}
		ebo(const Allocator& a) : Allocator(a) {}
	} ebo_;
	T* last_;
	T* eos_; 
};

template <class T, class A>
inline bool operator==(const pod_vector<T, A>& lhs, const pod_vector<T, A>& rhs) {
	return lhs.size() == rhs.size()
		&& std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <class T, class A>
inline bool operator!=(const pod_vector<T, A>& lhs, const pod_vector<T, A>& rhs) {
	return ! (lhs == rhs);
}

template <class T, class A> 
inline bool operator<(const pod_vector<T, A>& lhs, const pod_vector<T, A>& rhs) {
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <class T, class A> 
inline bool operator>(const pod_vector<T, A>& lhs, const pod_vector<T, A>& rhs) {
	return rhs < lhs;
}

template<class T, class A> 
inline bool operator<=(const pod_vector<T, A>& lhs, const pod_vector<T, A>& rhs) {  
	return !(rhs < lhs);
}

template<class T, class A> 
inline bool operator>=(const pod_vector<T, A>& lhs, const pod_vector<T, A>& rhs) {
	return !(lhs < rhs);
}

template<class T, class A> 
inline void swap(pod_vector<T, A>& lhs, pod_vector<T, A>& rhs) { 
	lhs.swap(rhs);
}

}

#endif 
