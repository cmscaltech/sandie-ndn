# XrdNdnOss plugin

## Logging using ndn-log
By default, only **FATAL** errors will be printed to stdout. Follow [ndn-log](https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html) to read about the different levels of logging and how to enable them.

To enable the entire levels of logging run the following command:
```bash
user@cms:~# export NDN_LOG="xrdndnconsumer*=TRACE:xrdndnproducer*=TRACE"
```
