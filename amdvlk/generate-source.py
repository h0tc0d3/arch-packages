#!/usr/bin/python

import os
import sys
import urllib.request
import xml.etree.ElementTree as xml


def main():

    if len(sys.argv) != 2:
        print(
            "\nUsage: ./{0} amdvlk_version\n\nExample: ./{0} 2022.Q2.1\n".format(
                os.path.basename(__file__)
            ),
            file=sys.stderr
        )
        sys.exit(1)

    try:

        version = sys.argv[1]

        fetch_url = "https://raw.githubusercontent.com/GPUOpen-Drivers/AMDVLK/v-{0}/default.xml".format(
            version)
        stream = urllib.request.urlopen(fetch_url)

        root = xml.fromstring(stream.read())
        git_url = root.find('remote').get('fetch')

        for repository in root.iter('project'):
            name = repository.get('name')
            commit = repository.get('revision')
            print("\"amdvlk/{1}::git+{0}/{1}#commit={2}\"".format(
                  git_url, name, commit))

    except Exception as error:
        print(error, file=sys.stderr)


if __name__ == "__main__":
    main()
