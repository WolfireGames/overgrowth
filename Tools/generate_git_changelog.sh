#!/bin/bash

#This command generates a change-list like list of all commits back until the last tag.
git log --pretty=format:"- %s%n%b" --since="$(git show -s --format=%ad `git rev-list --tags --max-count=1`)"
