/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    iterator_adapter.h

Authors:

    Sharvil Nanavati (sharvil) 2010-05-24

--*/

#pragma once

#include "base.h"

#include <iterator>

template <typename AssocType>
class value_iterator : public std::iterator <std::forward_iterator_tag, typename AssocType::mapped_type>
{
	public:
		typedef value_iterator <AssocType> self_type;
		typedef typename AssocType::iterator iterator_type;
		typedef typename AssocType::mapped_type value_type;

		value_iterator();
		value_iterator(iterator_type iterator);

		value_iterator & operator ++ ();
		value_iterator operator ++ (int);

		value_type & operator * () const;
		value_type * operator -> () const;

		bool operator == (const self_type & rhs) const;
		bool operator != (const self_type & rhs) const;

	private:
		iterator_type iIterator;
};

template <typename AssocType>
class const_value_iterator : public std::iterator <std::forward_iterator_tag, const typename AssocType::mapped_type>
{
	public:
		typedef const_value_iterator <AssocType> self_type;
		typedef typename AssocType::const_iterator iterator_type;
		typedef const typename AssocType::mapped_type value_type;

		const_value_iterator();
		const_value_iterator(iterator_type iterator);

		const_value_iterator & operator ++ ();
		const_value_iterator operator ++ (int);

		value_type & operator * () const;
		value_type * operator -> () const;

		bool operator == (const self_type & rhs) const;
		bool operator != (const self_type & rhs) const;

	private:
		iterator_type iIterator;
};

template <typename AssocType>
value_iterator <AssocType>::value_iterator()
{
}

template <typename AssocType>
value_iterator <AssocType>::value_iterator(iterator_type iterator)
	: iIterator(iterator)
{
}

template <typename AssocType>
value_iterator <AssocType> & value_iterator <AssocType>::operator ++ ()
{
	++iIterator;
	return *this;
}

template <typename AssocType>
value_iterator <AssocType> value_iterator <AssocType>::operator ++ (int)
{
	self_type ret = *this;
	++(*this);
	return ret;
}

template <typename AssocType>
typename value_iterator <AssocType>::value_type & value_iterator <AssocType>::operator * () const
{
	return iIterator->second;
}

template <typename AssocType>
typename value_iterator <AssocType>::value_type * value_iterator <AssocType>::operator -> () const
{
	return &iIterator->second;
}

template <typename AssocType>
bool value_iterator <AssocType>::operator == (const self_type & rhs) const
{
	return iIterator == rhs.iIterator;
}

template <typename AssocType>
bool value_iterator <AssocType>::operator != (const self_type & rhs) const
{
	return !(*this == rhs);
}

template <typename AssocType>
const_value_iterator <AssocType>::const_value_iterator()
{
}

template <typename AssocType>
const_value_iterator <AssocType>::const_value_iterator(iterator_type iterator)
	: iIterator(iterator)
{
}

template <typename AssocType>
const_value_iterator <AssocType> & const_value_iterator <AssocType>::operator ++ ()
{
	++iIterator;
	return *this;
}

template <typename AssocType>
const_value_iterator <AssocType> const_value_iterator <AssocType>::operator ++ (int)
{
	self_type ret = *this;
	++(*this);
	return ret;
}

template <typename AssocType>
typename const_value_iterator <AssocType>::value_type & const_value_iterator <AssocType>::operator * () const
{
	return iIterator->second;
}

template <typename AssocType>
typename const_value_iterator <AssocType>::value_type * const_value_iterator <AssocType>::operator -> () const
{
	return &iIterator->second;
}

template <typename AssocType>
bool const_value_iterator <AssocType>::operator == (const self_type & rhs) const
{
	return iIterator == rhs.iIterator;
}

template <typename AssocType>
bool const_value_iterator <AssocType>::operator != (const self_type & rhs) const
{
	return !(*this == rhs);
}
