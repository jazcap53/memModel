// Called by: FreeList::refresh(), InodeTable::assignInN(),
//            InodeTable::releaseInN(), InodeTable::nodeInUse(),
//            InodeTable::assignBlkN(), WipeList::isDirty()
template <typename T, typename U, typename V, T aSz, U bSz>
bool ArrBit<T,U,V,aSz,bSz>::test(V ix) const
{
    return arBt[ix / bSz].test(ix % bSz);
}

// Call set() on each bitset in arBt
template <typename T, typename U, typename V, T aSz, U bSz>
void ArrBit<T,U,V,aSz,bSz>::set()
{
    for (auto &b : arBt)
        b.set();    
}

// Called by: FreeList::refresh(), InodeTable::releaseInN(), 
//            WipeList::setDirty()
template <typename T, typename U, typename V, T aSz, U bSz>
void ArrBit<T,U,V,aSz,bSz>::set(V ix)
{
    arBt[ix / bSz].set(ix % bSz);    
}

// Called by: FreeList::refresh(), WipeList::clearArray()
template <typename T, typename U, typename V, T aSz, U bSz>
void ArrBit<T,U,V,aSz,bSz>::reset()
{
    for (auto &b : arBt)
        b.reset();    
}

// Called by: FreeList::getBlk(), InodeTable::assignInN()
template <typename T, typename U, typename V, T aSz, U bSz>
void ArrBit<T,U,V,aSz,bSz>::reset(V ix)
{
    arBt[ix / bSz].reset(ix % bSz);    
}

template <typename T, typename U, typename V, T aSz, U bSz>
auto ArrBit<T,U,V,aSz,bSz>::size() const -> U
{
    U ttlSz = static_cast<U>(0);
    for (const auto &a : arBt) 
        ttlSz += a.size();

    return ttlSz;
}

// Called by: WipeList::isRipe()
template <typename T, typename U, typename V, T aSz, U bSz>
auto ArrBit<T,U,V,aSz,bSz>::count() const -> V
{
    V ttlCt = static_cast<V>(0);
    for (const auto &a : arBt)
        ttlCt += a.count();

    return ttlCt;
}

template <typename T, typename U, typename V, T aSz, U bSz>
bool ArrBit<T,U,V,aSz,bSz>::all() const
{
    bool isAll = true;

    for (const auto &a : arBt)
        if (!a.all()) {
            isAll = false;
            break;
        }

    return isAll;
}

// Called by: FreeList::getBlk()
template <typename T, typename U, typename V, T aSz, U bSz>
bool ArrBit<T,U,V,aSz,bSz>::any() const
{
    bool isAny = false;

    for (const auto &a : arBt)
        if (a.any()) {
            isAny = true;
            break;
        }

    return isAny;    
}

template <typename T, typename U, typename V, T aSz, U bSz>
void ArrBit<T,U,V,aSz,bSz>::flip()
{
    for (auto &a : arBt)
        a.flip();
}

template <typename T, typename U, typename V, T aSz, U bSz>
void ArrBit<T,U,V,aSz,bSz>::flip(V ix)
{
    arBt[ix / bSz].flip(ix % bSz);        
}

template <typename T, typename U, typename V, T aSz, U bSz>
bool ArrBit<T,U,V,aSz,bSz>::none() const
{
    bool isNone = true;

    for (const auto &a : arBt)
        if (a.any()) {
            isNone = false;
            break;
        }

    return isNone;
}

// Note: non-standard implementation made private
// Called by: operator|=()
template <typename T, typename U, typename V, T aSz, U bSz>
const std::bitset<bSz> &ArrBit<T,U,V,aSz,bSz>::operator[](uint32_t i) const
{
    return arBt[i];
}

// Note: non-standard implementation made private
// Called by: operator|=()
template <typename T, typename U, typename V, T aSz, U bSz>
std::bitset<bSz> &ArrBit<T,U,V,aSz,bSz>::operator[](uint32_t i)
{
    return arBt[i];
}

// Called by: FreeList::refresh()
template <typename T, typename U, typename V, T aSz, U bSz>
ArrBit<T,U,V,aSz,bSz> ArrBit<T,U,V,aSz,bSz>::operator|=(const ArrBit
                                                        <T,U,V,aSz,bSz> &rhs)
{
    if (&rhs != this) {   // just to save time; a |= b has no effect when a == b
        for (uint32_t i = 0U; i != aSz; ++i)
            for (uint32_t j = 0U; j != bSz; ++j)
                if (rhs[i].test(j))
                    arBt[i].set(j);
    }

    return *this;
}
