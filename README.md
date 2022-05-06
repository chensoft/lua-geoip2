# lua-geoip2

Lua bindings for MaxMind GeoIP2 library

## Prepare

#### Debian/Ubuntu

```shell
apt install libmaxminddb-dev
```

#### MacOS

```shell
brew install libmaxminddb
```

#### Other OS

see https://github.com/maxmind/libmaxminddb

## Install

build with cmake or run `luarocks install geoip2`

## Interface

* open(your_geoip2_file_path)
* lookup(ip_address)

## Example

```lua
local mmdb = require("geoip2").open("/your_path/geoip2-city.mmdb")
local info = mmdb:lookup("142.250.66.36")

print(require("dkjson").encode(info))  -- luarocks install dkjson
```

## License

lua-geoip2 is released under the MIT License. See the LICENSE file for more information.

## Contact

You can contact me by email: admin@chensoft.com.