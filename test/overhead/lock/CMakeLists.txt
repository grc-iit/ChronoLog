add_executable(lock_overhead_test lock_overhead_test.cpp)
target_link_libraries(lock_overhead_test -lpthread -lrt)

add_test(NAME LockOverhead COMMAND lock_overhead_test)

