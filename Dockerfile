FROM gcc AS builder

WORKDIR /build

COPY ./main.c ./
COPY ./Makefile ./
RUN make

FROM debian AS runner

WORKDIR /app

COPY --from=builder /build/telnetter ./
COPY ./content ./content

ENV TZ Asia/Tokyo

CMD ["./telnetter", "2000"]
