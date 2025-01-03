# docker build -t rocketio/dev:20250101 -f dev.dockerfile .
FROM debian:bookworm-20241223 AS build
RUN apt-get update           &&\
  apt-get install -y wget      \
  cmake make                   \
  gcc g++                    &&\
  apt-get clean
WORKDIR /root
RUN wget -qO- https://github.com/Kitware/CMake/releases/download/v3.31.3/cmake-3.31.3-linux-x86_64.tar.gz |\
  tar xzv -C /root
RUN mv /root/cmake-3.31.3-linux-x86_64 /root/cmake

FROM debian:bookworm-20241223
RUN apt-get update                       &&\
  apt-get install -y apt-transport-https   \
  ca-certificates                          \
  gnupg2                                   \
  git wget                                 \
  make build-essential                     \
  python3 python3-pip                      \
  gcc g++ gdb                            &&\
  apt-get clean
# https://apt.llvm.org/
RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key  |\
  tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc            &&\
  apt-get update                                         &&\
  apt-get install -y libllvm-19-ocaml-dev                  \
  libllvm19                                                \
  llvm-19                                                  \
  llvm-19-dev                                              \
  llvm-19-doc                                              \
  llvm-19-examples                                         \
  llvm-19-runtime                                          \
  clang-19                                                 \
  clang-tools-19                                           \
  clang-19-doc                                             \
  libclang-common-19-dev                                   \
  libclang-19-dev                                          \
  libclang1-19                                             \
  clang-format-19                                          \
  python3-clang-19                                         \
  clangd-19                                                \
  clang-tidy-19
RUN find /usr/bin -regextype posix-extended  \
  -regex "^.*/(llvm|clang)-.*-19$"          |\
  while read bin;                            \
  do                                         \
  target=$(echo "$bin" | sed "s/-19$//");    \
  ln -sf "$bin" "$target";                   \
  done
COPY --from=build /root/cmake /root/cmake
RUN ln -sf /root/cmake/bin/cmake /usr/bin/cmake
RUN git clone https://github.com/include-what-you-use/include-what-you-use.git -b clang_19 &&\
  cd include-what-you-use                                                                  &&\
  mkdir build                                                                              &&\
  cd build                                                                                 &&\
  cmake -G "Unix Makefiles" -DCMAKE_PREFIX_PATH=/usr/lib/llvm-19 ..                        &&\
  make install                                                                             &&\
  cd ../..                                                                                 &&\
  rm -rf include-what-you-use
RUN pip install cmakelang cpplint --break-system-packages
