#ifndef ALIGNED_STORER_H
#define ALIGNED_STORER_H

#include <type_traits>

template<std::size_t Len1, std::size_t Align1 /* default alignment not implemented */>
class Aligned_storer
{
public:
   static constexpr std::size_t Len   = Len1;
   static constexpr std::size_t Align = Align1;
private:
   typename std::aligned_storage<Len1, Align1>::type storage;
};

#endif
