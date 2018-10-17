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




createdb("dbtest")
# success: (return root thing)
```
[{
    _id: 123,
}]


(dbtest) bla = []  # creates a primitives array


(dbtest) bla = id(4)
(dbtest) bla = name
(dbtest) bla = lables[0]
(dbtest) fetch()

(dbtest) labels.map(label => label.fetch()).map(label => label.owner.fetch())
(dbtest) labels.first(label => label.name == 'some label').customers.fetch()
(dbtest) id(123).fetch()

(dbtest) labels.push({owner: id(123), name: 'bla', })

(dbtest) thing(5).fetch('age')

[
    {
        _id: 123,
        age: 4
    }
]


dbtest.bla = dbtest.name


elem {
    items <item>: [{
        prop_name: x,
        val: null, true, false, "", [<primitives>], [<elems>], <elem>
    }]
}









thing(0).labels.


item(root)