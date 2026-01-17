# framedotOptions.cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Engine tunables (cache vars)
set(FRAMEDOT_MAX_INPUT_EVENTS "256" CACHE STRING "프레임당 입력 이벤트 최대 개수(InputQueue 용량)")
set(FRAMEDOT_INPUT_OVERFLOW_POLICY "1" CACHE STRING "입력 큐 오버플로 정책: 0=DropNewest, 1=DropOldest, 2=CoalesceMouseMove(예약)")