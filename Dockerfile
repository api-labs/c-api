FROM silkeh/clang:10 as builder

WORKDIR /usr/src/c-api
COPY server.c server.c

RUN clang -O3 server.c -pthread -o server

FROM debian:buster

WORKDIR /usr/src/c-api

COPY --chown=0:0 --from=builder /usr/src/c-api/server /usr/src/c-api