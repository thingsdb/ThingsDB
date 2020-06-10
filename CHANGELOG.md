# v0.9.3 (alpha)

* Added `emit(..)` function for broadcasting events, issue #31.
* Fixed error in generating a new type instance with enumerators, issue #33.
* Fixed invalid copy of list when using the `new(..)` function, issue #36.
* Added a new function `assign(..)`, issue #32.
* Fixed incorrect type count when using thing as variable, issue #42.
* Fixed missing Thing ID after receiving a new thing by an event, issue #41.
* Changed the return value for function `.del(..)`, issue #40. 
* Fixed instances may be in use while changing a type, issue #39.
* Fixed `list` or `set` may keep restriction after using mod_type(), issue #38.
* Allow the `mod` action for function `mod_type(..)` to use a closure and remove specification restriction, issue #34.

# v0.9.2

* Accept a closure when adding a type definition, issue #10.
* Initialize type using default values, issue #25.
* Added option to change default value for enum type, issue #26.
* Fixed iterating over a set returning `nil` instead of `0`, issue #29.
* Added `reduce(..)`, `some(..)` and `every(..)` for the `set` type, issue #24.


# v0.9.1

* Fixed wrong formatting thing with one item, issue #23.
* Add a rename action to the `mod_enum(..)` and `mod_type(..)` functions, issue #18.
* Support for minus (and plus) sign before expressions, issue #21.


# v0.9.0

* Added an *enum* type, issue #9.
* Variable in scope is not re-used, issue #16.
* Prevent creating type which name conflict with a build-in type or specification, issue #15.
* Existing array and set specifications are not updated after using mod_type(...), issue #14.
* No copy is created for an array when calling a closure bug, issue #13.
* Incorrect delete type after using mod_type(), issue #12.
* Typo in function "new" error message, issue #11.
* Return procedure definitions using a readable format enhancement, issue #7.


# v0.8.9

* Wrap() might fail after changing a Type, issue #6.


# v0.8.8

* Exclude backup files from ThingsDB backup file.


# v0.8.7

* Fixed connecting nodes.
* Fixed synchronizing on alphine linux.
* Added restore() function.