FROM debian:stable-slim

RUN apt-get update
RUN apt-get install -y make gcc
