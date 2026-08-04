#!/bin/bash

DATE=$(date +%F)

pg_dump \
    -U postgres \
    percival | gzip \
    > /home/ubuntu/backups/percival_$DATE.sql.gz

find /home/ubuntu/backups -type f -mtime +30 -delete
