# docker build -t rocketio/dev:20250118 -f dev.dockerfile .
FROM debian:bookworm-20241223
# https://docs.docker.com/reference/dockerfile/#shell
# The `SHELL` instruction allows the default shell used for
# the shell form of commands to be overridden.
# https://github.com/progrium/bashstyle
# Always use `set -eo pipefail`.
# Fail fast and be aware of exit codes.
SHELL ["/bin/bash", "-euo", "pipefail", "-c"]
RUN apt-get update                                   &&\
  apt-get install -y --no-install-recommends           \
  apt-transport-https=2.6.1 ca-certificates=20230311   \
  gnupg2=2.2.40-1.1                                    \
  git=1:2.39.5-0+deb12u1 wget=1.21.3-1+b2              \
  autoconf=2.71-3 automake=1:1.16.5-1.3                \
  build-essential=12.9 libtool=2.4.7-7~deb12u1         \
  cmake=3.25.1-1 make=4.3-4.1                          \
  python3=3.11.2-1+b1 python3-pip=23.0.1+dfsg-1        \
  gcc=4:12.2.0-3 g++=4:12.2.0-3 gdb=13.1-3           &&\
  apt-get clean                                      &&\
  rm -rf /var/lib/apt/lists/*
# https://apt.llvm.org/
RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key    |\
  tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc              &&\
  apt-get update                                           &&\
  apt-get install -y --no-install-recommends                 \
  libllvm-19-ocaml-dev=1:19.1.4-1~deb12u1                    \
  libllvm19=1:19.1.4-1~deb12u1                               \
  llvm-19=1:19.1.4-1~deb12u1                                 \
  llvm-19-dev=1:19.1.4-1~deb12u1                             \
  llvm-19-doc=1:19.1.4-1~deb12u1                             \
  llvm-19-examples=1:19.1.4-1~deb12u1                        \
  llvm-19-runtime=1:19.1.4-1~deb12u1                         \
  clang-19=1:19.1.4-1~deb12u1                                \
  clang-tools-19=1:19.1.4-1~deb12u1                          \
  clang-19-doc=1:19.1.4-1~deb12u1                            \
  libclang-common-19-dev=1:19.1.4-1~deb12u1                  \
  libclang-rt-19-dev=1:19.1.4-1~deb12u1                      \
  libclang-19-dev=1:19.1.4-1~deb12u1                         \
  libclang1-19=1:19.1.4-1~deb12u1                            \
  clang-format-19=1:19.1.4-1~deb12u1                         \
  python3-clang-19=1:19.1.4-1~deb12u1                        \
  clangd-19=1:19.1.4-1~deb12u1                               \
  clang-tidy-19=1:19.1.4-1~deb12u1                         &&\
  apt-get clean                                            &&\
  rm -rf /var/lib/apt/lists/*
RUN find /usr/bin -regextype posix-extended  \
  -regex "^.*/(llvm|clang).*-19$"           |\
  while read bin;                            \
  do                                         \
  target=$(echo "$bin" | sed "s/-19$//");    \
  ln -sf "$bin" "$target";                   \
  done
RUN git clone https://github.com/include-what-you-use/include-what-you-use.git -b clang_19 &&\
  cd include-what-you-use                                                                  &&\
  mkdir build                                                                              &&\
  cd build                                                                                 &&\
  cmake -G "Unix Makefiles" -DCMAKE_PREFIX_PATH=/usr/lib/llvm-19 ..                        &&\
  make install                                                                             &&\
  cd ../..                                                                                 &&\
  rm -rf include-what-you-use
RUN pip install --break-system-packages --no-cache-dir cmakelang==0.6.13 cpplint==2.0.0
