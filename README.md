# telnet.yude.jp

Tiny telnet server written in C, hosted on `telnet://telnet.yude.jp`.

## Deploy

### Fly

You can deploy this on [Fly](https://fly.io), free of charge. Fly also provides free global IPv4 & IPv6 address.\
You can refer to `fly.toml` for configuration.

### Docker Compose

* `docker compose up -d`

## Usage

* All contents placed under `content` directory will be visible by running its file name (w/o extension) as a command.
* `content/message.txt` will be shown at first to remote users.


## License

MIT license.
