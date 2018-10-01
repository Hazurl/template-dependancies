#include <iostream>

struct Void {};

template<typename...>
struct Failure : std::integral_constant<bool, false> {};

template<typename...Ts>
static constexpr auto Failure_v = Failure<Ts...>::value;

template<typename T>
struct Type { using type = T; };

template<std::size_t ID>
struct System {
    static constexpr std::size_t id = ID;
};

template<typename S, typename...Deps>
struct Dependancies {};

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

template<typename...Ss>
struct Systems : List<GetSystem_t<Ss>...> {};

template<typename...Ss>
struct Systems<List<Ss...>> : List<GetSystem_t<Ss>...> {};

template<typename...Ss>
std::ostream& operator << (std::ostream& os, List<Ss...>) {
    return ((os << "[ ") << ... << Ss::id) << " ]";
}

int main() {

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

    std::cout << D0{} << '\n';

    std::cout << Systems<Deps>{} << '\n';

    return 0;
}