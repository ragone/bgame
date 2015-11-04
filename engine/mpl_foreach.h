#pragma once

#include <algorithm>
#include <utility>
#include <tuple>
#include <memory>

namespace engine {
namespace ecs_detail {

template <typename Tuple, typename F, std::size_t ...Indices>
void for_each_impl ( Tuple&& tuple, F&& f, std::index_sequence<Indices...> )
{
     using swallow = int[];
     ( void ) swallow {1,
                       ( f ( std::get<Indices> ( std::forward<Tuple> ( tuple ) ) ), void(), int{} )...
                      };
}

template <typename Tuple, typename F>
void for_each ( Tuple&& tuple, F&& f )
{
     constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
     for_each_impl ( std::forward<Tuple> ( tuple ), std::forward<F> ( f ),
                     std::make_index_sequence<N> {} );
}
  
}
}