PICO=../../zenoh-pico
cflow  --format=posix --omit-arguments -i_ $* \
      ${PICO}/examples/net/zn_sub.c  ${PICO}/src/*.c ${PICO}/src/*/*.c ${PICO}/src/net/unix/*.c   -DZENOH_LINUX -DZENOH_TRANSPORT_TCP=ON -I ${PICO}/include/ 
