#!/bin/sh

#git_version=$(git describe --abbrev=4 --dirty --always --tags)
#
#if [ -z "$git_version" ]; then
#  echo "Git version string will be left unchanged."
#  exit
#fi

git_version=`fgrep Revision < netmist_version.c | cut -d: -f2 | cut -d$ -f 1`
echo "Updating netmist_version.c"

git_sha=$(git rev-parse HEAD)
git_date=$(date)
#git_version=`echo $git_version | cut -d'-' -f1`
echo $git_version
exit

#
# Below is now done in git_commit.sh
#
# create version file
#echo "Updating benchmark version information to Git version "$git_version
#echo "#include \"netmist_version.h\"" > netmist_version.c
#echo "char *git_date = \"$git_date\";" >> netmist_version.c
#echo "char *git_sha = \"$git_sha\";" >> netmist_version.c
#echo "char *git_version = \"\$Revision: $git_version $\";" >> netmist_version.c
cat netmist_version.c
