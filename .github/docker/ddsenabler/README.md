# DDS ENABLER TEST DOCKER

In order to build this docker image, use command in current directory:

```sh
docker build --rm -t ddsenabler_test:some_tag --build-arg "fastcdr_branch=master" --build-arg "fastdds_branch=master" --build-arg "devutils_branch=main" --build-arg "ddspipe_branch=tmp/dds_enabler" --build-arg "ddsenabler_branch=CI" .
```
