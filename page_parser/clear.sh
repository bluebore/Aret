#!/bin/sh

./tera_client scan TestTagTable "" "" | awk '{print $1}' > keys
./tera_client delete_file TestTagTable keys
