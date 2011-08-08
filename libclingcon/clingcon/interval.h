#pragma once


#include <list>
#include <iostream>

template <class T>
struct Interval
{
	Interval(T const &left, T const &right) : left(left), right(right) { }
	T left;
	T right;
};

template <class T>
class IntervalSet
{
public:
	typedef Interval<T> ValueType;
	typedef std::list<ValueType> IntervalList;
        typedef typename IntervalList::iterator Iterator;
        typedef typename IntervalList::const_iterator ConstIterator;


public:
	ConstIterator begin() const
	{
		return list_.begin();
	}

	ConstIterator end() const
	{
		return list_.end();
	}

        typename IntervalList::size_type size() const
        {
            return list_.size();
        }

	void unite(const ValueType &v)
	{
		unite(v, list_.begin());
	}

	void unite(const IntervalSet<T> &iv)
	{
		Iterator j = list_.begin();
		for (ConstIterator i = iv.list_.begin(); i != iv.list_.end(); i++)
		{
			j = unite(*i, j);
		} 
	}

	void intersect(const IntervalSet<T> &iv)
	{
		Iterator j = list_.begin();
		for (ConstIterator i = iv.list_.begin(); i != iv.list_.end() && j != list_.end(); )
		{
			if (i->left >= j->right)
			{
				j = list_.erase(j);
			}
			else if(i->right <= j->left)
			{
				i++;
			}
			else
			{

                                if (i->right < j->right)
                                {
                                    ValueType add(std::max(j->left, i->left), i->right);
                                    j = list_.insert(j,add);
                                    ++j;
                                    ++i;

                                }
                                else
                                {
                                    j->left  = std::max(j->left, i->left);
                                    j->right;// = std::min(j->right, i->right);
                                    j++;
                                }
			}
		}
		list_.erase(j, list_.end());
	}

	void complement(ValueType const &iv)
	{
		T left = iv.left;
		for (Iterator j = list_.begin(); j != list_.end(); )
		{
			T right  = std::min(j->right, iv.right);
                        if (j->left <= left)
			{
				j = list_.erase(j);
			}
			else
			{
				j->right = j->left;
				j->left  = left;
				j++;
			}
			left = right;
		}
		if (left < iv.right) { list_.push_back(ValueType(left, iv.right)); }
	}

private:
	Iterator unite(const ValueType &v, const Iterator &it)
	{
		for (Iterator j = it; j != list_.end(); j++)
		{
			if (v.left < j->left)
			{
				if (v.right < j->left) { return list_.insert(j, v); }
				else
				{
					j->left = v.left;
					j->right = std::max(j->right, v.right);
					fix(j);
					return j;
				}
			}
			else if (v.left <= j->right)
			{
				j->right = std::max(j->right, v.right);
				fix(j);
				return j;
			}

		}
		list_.push_back(v);
	}

	void fix(const Iterator &j)
	{
		Iterator i = j;
		Iterator k = ++i;
		for (; k != list_.end() && j->right >= k->left; k++) { };
		j->right = std::max(j->right, (--k)->right);
		list_.erase(i, ++k);
	}

private:
	IntervalList list_;
};

template <class T>
std::ostream &operator <<(std::ostream &out, IntervalSet<T> const &set)
{
	for(typename IntervalSet<T>::ConstIterator i = set.begin(); i != set.end(); i++)
	{
		out << "[" << i->left << ", " << i->right << "] ";
	}
	return out;
}
/*
  //unit tests
int main()
{
	IntervalSet<int> a, b, c, d;
	a.unite(Interval<int>(3,4));
	a.unite(Interval<int>(6,7));
	a.unite(Interval<int>(8,10));
	a.unite(Interval<int>(11,14));

	b.unite(Interval<int>(1,2));
	b.unite(Interval<int>(5,6));
	b.unite(Interval<int>(9,12));
	b.unite(Interval<int>(13,15));

	c.unite(Interval<int>(1,4));
	c.unite(Interval<int>(5,8));

	std::cout << a << std::endl;;
	std::cout << b << std::endl;;
	a.unite(b);
	std::cout << a << std::endl;;
	a.complement(Interval<int>(0,9));
	std::cout << a << std::endl;;
	d = a;
	a.intersect(b);
	std::cout << a << std::endl;;
	d.intersect(c);
	std::cout << d << std::endl;;
}
*/
