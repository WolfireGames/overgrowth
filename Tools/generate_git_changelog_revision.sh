#!/bin/bash

#This command generates a change-list like list of all commits back until a revision.
revision=$1

git log --pretty=format:"- %s%n%b" --since="$(git show -s --format=%ad $revision)"