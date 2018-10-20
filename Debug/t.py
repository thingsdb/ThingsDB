#
# Panelen op dakkapel?
# Wat als het niet kan/past?
# Hoe zit het met garantie op opbrengst?
# Belasting voordeel?
#
import os
import subprocess

with open(os.devnull, 'w') as fnull:
    try:
        subprocess.call(['valgrindd'], stdout=fnull, stderr=fnull)
    except OSError as e:
        if e.errno == os.errno.ENOENT:
            print('valgrind not found')


p = subprocess.Popen(
    ['siridb-server', '--version'],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE)

output, err = p.communicate()
print(output.decode().splitlines())




# creates a database
{target: None, query: ..., blobs: []}
create_db(dbtest)
{
    '$id': 123,
    '$path': '.00000000001_'
    '$name': 'dbtest'
}

fetch(databases)

# get all databases
databases()
{
    'databases': [{

    }]
}

# create a things array
{db: "dbtest", cmd: ..., blobs: []}
people = [{id: 1, name: 'iris'}]
{
    '$id': 123,
    'people': [{
        '$id': 124,
        '$all': True,
        'id': 1,
        'name': 'iris'
    }]
}
# watchers
{
    '$id': 123,
    '$event': 26,
    'people': [{
        '$id': 124,
    }]
}

.thing()

# creates a things array
labels = []
{
    '$id': 123,
    'labels': []
}
# watchers
{
    '$id': 123,
    '$event': 27,
    'labels': []
}

# creates a thing
settings = {color: 'blue', debugMode: true}
{
    '$id': 123,
    'settings': {
        '$id': 125,
        '$all': True,
        'color': 'blue',
        'debugMode': True
    }
}
# watchers
{
    '$id': 123,
    '$event': 28,
    'settings': {
        '$id': 125,
    }
}

# $(124) could also be something like people) and labels will be turned into a things array.
# converting to another type array is only possible from things to primitives, but only when an array is empty.
# All nodes are aware of things arrays, so once primitive, not all nodes are aware of the state.
labels.push({id: 1, name: 'testlabel', owner: thing(124)})
{
    '$id': 123,
    '$event': 29,
    '$array': {
        'labels' {
            'push': [{
                '$id': 126,
                '$all': True,
                'id': 1,
                'name': 'testlabel',
                'owner': {
                    '$id': 124,
                }
            }]
        }
    }
}
# watchers
{
    '$id': 123,
    '$event': 29,
    '$array': {
        'labels' {
            'push': [{
                '$id': 126,
            }]
        }
    }
}

# watch based on an id
$id(124).watch()
{
    '$id': 124,
    '$event': 30,
    '$all': True,
    'id': 1,
    'name': 'iris'
}

# watch by name selecting and only one specific property
people[0].watch(name)
{
    '$id': 124,
    '$watch': ['name']
    '$event': 31,
    'name': 'iris'
}


# change settings color
settings.color = 'red'
{
    '$id': 125,
    'color': 'red'
}
# watchers
{
    '$id': 125,
    '$event': 33,
    'color': 'red'
}


people.map(person => person.badges = [])
{
    '$id': 123,
    'people': [{
        '$id': 124,
        'bages': []
    }]
}




labels.map(label => label.fetch())
{
    '$id': 123,
    'labels': [{
        '$id': 123,
        '$all': True,
        ...
    }]
}

hoi.map(label => label.badge[0]
# fetch() is only available on things
settings.color











thing(0).labels.


item(root)