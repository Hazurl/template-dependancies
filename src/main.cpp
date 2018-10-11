#include <iostream>
#include <type_traits>

template<typename...>
struct Failure : std::integral_constant<bool, false> {};

template<typename...Ts>
static constexpr auto Failure_v = Failure<Ts...>::value;

template<typename T>
struct Type { using type = T; };

struct Void : Type<Void> {};

template<std::size_t ID>
struct System {
    static constexpr std::size_t id = ID;
};

template<typename S, typename...Deps>
struct Dependancies {};

template<typename S, typename...Deps>
struct ReverseDependancies {};

template<typename T>
struct GetSystem : Void {
    static_assert(Failure_v<T>, "Not a Dependancy");
};

template<typename S, typename...Ds>
struct GetSystem<Dependancies<S, Ds...>> : Type<S> {};

template<typename T>
using GetSystem_t = typename GetSystem<T>::type;

template<typename S, typename...Deps>
std::ostream& operator << (std::ostream& os, Dependancies<S, Deps...>) {
    if constexpr(sizeof...(Deps) == 0)
        return os << S::id;
    else
        return ((os << S::id << "[ ") << ... << Deps{}) << " ]";
}

template<typename...Ts>
struct List {};
/*
template<typename...Ss>
struct Systems : List<GetSystem_t<Ss>...> {};

template<typename...Ss>
struct Systems<List<Ss...>> : List<GetSystem_t<Ss>...> {};

template<typename...Ss>
std::ostream& operator << (std::ostream& os, List<Ss...>) {
    return ((os << "[ ") << ... << Ss::id) << " ]";
}
*/
template<typename...>
struct Show;

/*
    Type :: a -> strict a
*/

template<typename A>
using Unwrap = typename A::type;


/*
    Entry :: a x b -> (a, b)
*/

template<typename A, typename B>
struct Entry {};

/*
    Key :: (a, b) -> a
*/

template<typename E>
struct Key : Void {
    static_assert(Failure_v<E>, "Not an Entry");
};

template<typename K, typename V>
struct Key<Entry<K, V>> : Type<K> {};

template<typename E>
using Key_t = Unwrap<Key<E>>;

/*
    Value :: (a, b) -> b
*/

template<typename E>
struct Value : Void {
    static_assert(Failure_v<E>, "Not an Entry");
};

template<typename K, typename V>
struct Value<Entry<K, V>> : Type<V> {};

template<typename E>
using Value_t = Unwrap<Value<E>>;

/*
    Push :: List a x a -> List a
    Push xs x = x : xs
*/

template<typename L, typename E>
struct Push : Void {
    static_assert(Failure_v<L, E>, "Not a List");
};

template<typename L, typename X>
using Push_t = Unwrap<Push<L, X>>;

template<typename...Es, typename E>
struct Push<List<Es...>, E> : Type<List<E, Es...>> {};



/*
    InsertUnique :: List a x a -> List a
    InsertUnique [] y = [y]
    InsertUnique (x:xs) y = if x == y then (x:xs) else x : (InsertUnique xs y)
*/

template<typename L, typename X>
struct InsertUnique : Void {
    static_assert(Failure_v<L, X>, "Not a List");
};

template<typename L, typename X>
using InsertUnique_t = Unwrap<InsertUnique<L, X>>;

template<typename X>
struct InsertUnique<List<>, X> : Type<List<X>> {};

template<typename...Xs, typename X>
struct InsertUnique<List<X, Xs...>, X> : Type<List<X, Xs...>> {};

template<typename Y, typename...Xs, typename X>
struct InsertUnique<List<Y, Xs...>, X> : Push<InsertUnique_t<List<Xs...>, X>, Y> {};

/*

    MapAt :: List (a, b) x a x (b -> b) -> List (a, b)
    MapAt [] _ _ = []
    MapAt (x:xs) y f = if (fst x) == y then (fst x, f (snd x)) : xs else x : MapAt xs y f

*/

template<typename L, typename X, template<typename> typename F> 
struct MapAt : Void {
    static_assert(Failure_v<L, X>, "Not a List");
};

template<typename L, typename X, template<typename> typename F> 
using MapAt_t = Unwrap<MapAt<L, X, F>>;

template<typename X, template<typename> typename F> 
struct MapAt<List<>, X, F> : Type<List<>> {};

template<typename Y, typename...Xs, typename X, template<typename> typename F> 
struct MapAt<List<Y, Xs...>, X, F> : 
    Type<
        std::conditional_t<std::is_same_v<Key_t<Y>, X>,
            // then
            List<Entry<Key_t<Y>, F<Value_t<Y>>>, Xs...>,
            // else
            Push_t<MapAt_t<List<Xs...>, X, F>, Y>
    >> {};


/*
    Map :: List a x (a -> b) -> List b 
    Map [] f = []
    Map (x:xs) f = f x : Map xs f
*/

template<typename L, template<typename> typename F>
struct Map : Void {
    static_assert(Failure_v<L>, "Not a list");
};

template<typename L, template<typename> typename F>
using Map_t = Unwrap<Map<L, F>>;

template<typename X, typename...Xs, template<typename> typename F>
struct Map<List<X, Xs...>, F> : Push<Map_t<List<Xs...>, F>, F<X>> {}; 

template<template<typename> typename F>
struct Map<List<>, F> : Type<List<>> {};

/*
    EntryMap :: (a, b) x (a x b -> c) -> c
    EntryMap (a, b) f = f a b
*/

template<typename E, template<typename, typename> typename F>
struct EntryMap : Void {
    static_assert(Failure_v<E>, "Not an entry");
};

template<typename K, typename V, template<typename, typename> typename F>
struct EntryMap<Entry<K, V>, F> : Type<F<K, V>> {};

template<typename E, template<typename, typename> typename F>
using EntryMap_t = Unwrap<EntryMap<E, F>>;

/*
    Curry :: (a x b -> c) x a -> b -> c
    Curry f a = \b -> f a b
*/

template<template<typename, typename> typename F, typename A>
struct Curry {
    template<typename B>
    using type = F<A, B>;
};

/*
    MapEntries :: List (a, b) x (a x b -> c) -> c
    MapEntries [] f = []
    MapEntries ((a, b):xs) f = f a b : MapEntries xs f
*/

template<typename L, template<typename, typename> typename F>
struct MapEntries : Void {
    static_assert(Failure_v<L>, "Not a list");
};

template<typename L, template<typename, typename> typename F>
using MapEntries_t = Unwrap<MapEntries<L, F>>;

template<typename X, typename...Xs, template<typename, typename> typename F>
struct MapEntries<List<X, Xs...>, F> : Push<MapEntries_t<List<Xs...>, F>, EntryMap_t<X, F>> {}; 

template<template<typename, typename> typename F>
struct MapEntries<List<>, F> : Type<List<>> {};

/*
    Fold :: List a x (a x b -> b) x b -> b
    Fold [] f a = a
    Fold (x:xs) f a = Fold xs f (f x a)
*/

template<typename L, template<typename, typename> typename F, typename A>
struct Fold : Void {
    static_assert(Failure_v<L, A>, "Not a list");
};

template<typename L, template<typename, typename> typename F, typename A>
using Fold_t = Unwrap<Fold<L, F, A>>;

template<template<typename, typename> typename F, typename A>
struct Fold<List<>, F, A> : Type<A> {};

template<typename X, typename...Xs, template<typename, typename> typename F, typename A>
struct Fold<List<X, Xs...>, F, A> : Fold<List<Xs...>, F, F<X, A>> {};

/*
    Flip :: (a x b -> c) -> (b x a -> c)
    Flip f = \b a -> f a b
*/

template<template<typename, typename> typename F>
struct Flip {
    template<typename A, typename B>
    using type = F<B, A>;
};

/*
    InsertUniqueSystem :: System -> System x List (System, List System) -> List (System, List System)
    InsertUniqueSystem s x ys = MapAt ys x (Curry (Flip InsertUnique) s)
    -- InsertUniqueSystem s x ys <=> ys[x].insert_unique(s)
*/

template<typename S, typename X, typename L>
struct InsertUniqueSystemImpl : MapAt<L, X, Curry<Flip<InsertUnique_t>::template type, S>::template type> {};

template<typename S, typename X, typename L>
using InsertUniqueSystemImpl_t = Unwrap<InsertUniqueSystemImpl<S, X, L>>;

template<typename S>
struct InsertUniqueSystem {
    template<typename X, typename L>
    using type = InsertUniqueSystemImpl_t<S, X, L>;
};

/*
    Insert :: System x List System x List (System, List System) -> List (System, List System)
    Insert s xs ys = Fold xs (InsertUniqueSystem s) ys
*/

template<typename S, typename L, typename A>
struct Insert : Fold<L, InsertUniqueSystem<S>::template type, A> {};

template<typename S, typename L, typename A>
using Insert_t = Unwrap<Insert<S, L, A>>;

/*
    InsertEntry :: (System, List System) x List (System, List System) -> List (System, List System)
    InsertEntry (s, xs) ys = Insert s xs ys
*/

template<typename E, typename L>
using InsertEntry = Insert<Key_t<E>, Value_t<E>, L>;

template<typename E, typename L>
using InsertEntry_t = Unwrap<InsertEntry<E, L>>;

/*
    InsertAll :: List (System, List System) -> List (System, List System)
    InsertAll xs = Fold xs f List<>
*/

template<typename L, typename A = List<>>
using InsertAll = Fold<L, InsertEntry_t, A>;

template<typename L, typename A = List<>>
using InsertAll_t = Unwrap<InsertAll<L, A>>;

/*
    InitReverseDepandancies :: List System -> List (System, List System)
    InitReverseDepandancies xs = Map xs SingletonReverseDepandancies_t
        where
            SingletonReverseDepandancies_t x = (x, List<>)
*/

template<typename S>
using SingletonReverseDepandancies_t = Entry<S, List<>>;

template<typename L>
using InitReverseDepandancies_t = Map_t<L, SingletonReverseDepandancies_t>; 

/*
    GetReverseDepandancies :: List System -> List (System, List System) -> List (System, List System)
    GetReverseDepandancies xs gs = InsertAll gs (InitReverseDepandancies xs)
*/

template<typename S, typename G>
using GetReverseDepandancies = InsertAll<G, InitReverseDepandancies_t<S>>;

template<typename S, typename G>
using GetReverseDepandancies_t = Unwrap<GetReverseDepandancies<S, G>>;

/*
    PushKeyIfEmpty :: (a, List b) x List a -> List a
    PushKeyIfEmpty (a, xs) ys = if empty xs then a : ys else ys
*/

template<typename L>
struct Empty : std::false_type {};

template<typename...Xs>
struct Empty<List<Xs...>> : std::integral_constant<bool, (sizeof...(Xs) == 0)> {};

template<typename L>
static constexpr bool Empty_v = Empty<L>::value;

template<typename E, typename L, bool = Empty_v<Value_t<E>>>
struct PushKeyIfEmpty : Type<L> {};

template<typename E, typename L>
struct PushKeyIfEmpty<E, L, true> : Push<L, Key_t<E>> {};

template<typename E, typename L>
using PushKeyIfEmpty_t = Unwrap<PushKeyIfEmpty<E, L>>;

/*
    GetRoots :: List (System, List System) -> List System
    GetRoots [] = []
    GetRoots (x:xs) = if empty snd x then (fst x) : GetRoots xs else GetRoots xs
*/

template<typename L>
using GetRoots = Fold<L, PushKeyIfEmpty_t, List<>>;

template<typename L>
using GetRoots_t = Unwrap<GetRoots<L>>;

using S0 = System<0>;
using S1 = System<1>;
using S2 = System<2>;
using S3 = System<3>;
using S4 = System<4>;
using S5 = System<5>;
using S6 = System<6>;

using D6 = Dependancies<S6>;
using D4 = Dependancies<S4>;
using D3 = Dependancies<S3>;
using D2 = Dependancies<S2>;
using D5 = Dependancies<S5, D6>;
using D1 = Dependancies<S1, D4, D5>;
using D0 = Dependancies<S0, D1, D2, D3>;

using Deps = List<D0, D1, D2, D3, D4, D5, D6>;
using Systems = List<S0, S1, S2, S3, S4, S5, S6>;

using E0 = Entry<S0, List<S1, S2, S3>>;
using E1 = Entry<S1, List<S4, S5>>;
using E5 = Entry<S5, List<S6>>;

using Es = List<E0, E1, E5>;

template<typename L>
using Insert_S5 = InsertUnique_t<L, S5>;

template<typename K, typename V>
using Insert_S5_InEntry = Entry<K, Insert_S5<V>>;

int main() {

/*

    Systems := List<System>
    Depandancies := List<System -> List<System>>

    =>

    Reverse Depandancies := List<System -> List<System>>
    #   for system, depandancies in Depandancies
    #       for depandancy in depandancies
    #           Map at depandancy in Reverse Depandancies with
    #               ds => 
    #                   if system not in ds
    #                       insert system in ds

    # Insert :: System x List System x List (System, List System) -> List (System, List System)
    # Insert s xs ys = Fold xs (f s) ys
    #   where 
    #       f :: System -> System x List (System, List System) -> List (System, List System)
    #       f s x ys = MapAt ys x (Curry (Flip InsertUnique) s)
    #       -- f s x ys <=> ys[x].insert_unique(s)

    # InsertAll :: List (System, List System) -> List (System, List System)
    # InsertAll xs = Fold xs f List<>
    #   where
    #       f :: (System, List System) x List (System, List System) -> List (System, List System)
    #       f (s, xs) ys = Insert s xs ys

    =>

    Heights := List<System -> Height>
    #   for system, reverse depandancies in Reverse Depandancies
    #       Set at system in Heights to size of reverse depandancies

    #   GetRoots...

    #   GetHeights :: List (System, List System) x List (System, List System) -> List (System, std::size_t)
    #   GetHeights ds rs = GetHeightsOf (GetRoots rs) ds List<> 0

    #   GetHeightsOf :: List System x List (System, List System) x List (System, std::size_t) x std::size_t -> List (System, std::size_t)
    #   GetHeightsOf ss ds hs h = if empty ss then hs else GetHeightsOf (Fold ss (j ds) List<>) ds (Fold ss (f h) hs) (h + 1)
    #       where
    #           f :: std::size_t -> System x List (System, std::size_t) -> List (System, std::size_t)
    #           f h s hs = MapAt s hs (g h)
    #           g :: std::size_t -> std::size_t -> std::size_t
    #           g h i = max h i

*/

    Show< Systems > $0;
    Show< Push_t<Systems, int> > $1;
    Show< InsertUnique_t<Systems, int> > $2;
    Show< InsertUnique_t<Systems, S2> > $3;

    Show< Es > $4;
    Show< MapAt_t<Es, S2, Insert_S5> > $5;
    Show< MapAt_t<Es, S1, Insert_S5> > $6;
    Show< MapAt_t<Es, S0, Insert_S5> > $7;
    Show< MapAt_t< MapAt_t<Es, S0, Insert_S5>, S0, Insert_S5 > > $8;

    Show< Es > $9;
    Show< MapEntries_t<Es, Insert_S5_InEntry> > $10;

    Show< Fold_t<List<S0, S1, S2>, Flip<Push_t>::type, List<S6>> > $11;
    
    Show< 
        InsertUniqueSystemImpl_t<
            S0, 
            S2, 
            List<
                Entry<
                    S1, 
                    List<
                        S2, 
                        S3
                    >
                >,
                Entry<
                    S4, 
                    List<
                        S0, 
                        S6
                    >
                >
            >
        >
    > $12;

    Show< 
        InsertUniqueSystem<S0>::type<
            S4, 
            List<
                Entry<
                    S1, 
                    List<
                        S2, 
                        S3
                    >
                >,
                Entry<
                    S4, 
                    List<
                        S0, 
                        S6
                    >
                >
            >
        >
    > $13;

    Show<
        Insert_t<
            S0,
            List<S1, S2>,
            List<
                Entry<
                    S1,
                    List<>
                >,
                Entry<
                    S3,
                    List<S6, S5>
                >
            >
        >
    > $14;

    using Graph = List<
        Entry<S0, List<>>,
        Entry<S1, List<S0, S3>>,
        Entry<S2, List<S3>>
    >;

    Show<
        Graph
    > $15;

    Show<
        InsertAll_t<Graph, List<Entry<S0, List<>>, Entry<S1, List<>>, Entry<S2, List<>>, Entry<S3, List<>>>>
    > $16;

    Show<
        InitReverseDepandancies_t<Systems>
    > $17;

    Show<
        GetReverseDepandancies_t<Systems, Graph>
    > $18;

    Show<
        GetReverseDepandancies_t<Systems, Es>
    > $19;

    Show<
        GetRoots_t<GetReverseDepandancies_t<Systems, Es>>
    > $20;

    return 0;
}