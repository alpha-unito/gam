set(TESTS pingpong
          simple_public simple_private simple_publish
          non_trivially_copyable)

# TODO mtu

foreach(t ${TESTS})
    add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} gam)
endforeach(t)