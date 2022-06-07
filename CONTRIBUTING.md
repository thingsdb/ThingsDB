# Contributing to ThingsDB

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

The following is a set of guidelines for contributing to ThingsDB and its packages, which are hosted in the [ThingsDB Organization](https://github.com/thingsdb) on GitHub. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

#### Table Of Contents

[Code of Conduct](#code-of-conduct)

[I don't want to read this whole thing, I just have a question!!!](#i-dont-want-to-read-this-whole-thing-i-just-have-a-question)

[What should I know before I get started?](#what-should-i-know-before-i-get-started)
  * [ThingsDB and related projects](#thingsdb-and-related-projects)

[How Can I Contribute?](#how-can-i-contribute)
  * [Reporting Bugs](#reporting-bugs)
  * [Suggesting Enhancements](#suggesting-enhancements)
  * [Your First Code Contribution](#your-first-code-contribution)
  * [Pull Requests](#pull-requests)

[Styleguides](#styleguides)
  * [C Styleguide](#c-styleguide)
  * [Python Styleguide](#python-styleguide)
  * [Documentation](#documentation)
  
## Code of Conduct

This project and everyone participating in it is governed by the [ThingsDB Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to [info@thingsdb.net](mailto:info@thingsdb.net).

## I don't want to read this whole thing I just have a question!!!

> **Note:** Please don't file an issue to ask a question.

We have an official message board where the community chimes in with helpful advice if you have questions.

* [GitHub Discussions, the officialThingsDB message board](https://github.com/thingsdb/ThingsDB/discussions)

## What should I know before I get started?

### ThingsDB and related projects

ThingsDB is an open source project &mdash; with several [repositories](https://github.com/thingsdb).

Here's a list with a few of them:

* [thingsdb/ThingsDB](https://github.com/athingsdb/ThingsDB) -ThingsDB Core! This is the code that runs ThingsDB and is written using the `C` language.
* [ThingsDocs](https://github.com/thingsdb/ThingsDocs) - The official ThingsDB documentation.
* [ThingsGUI](https://github.com/thingsdb/ThingsGUI) - A GUI for managing ThingsDB. This project contains frontend code written in `JavaScript`/`React` and a webserver component written in `Go`.

### ThingsDB and Modules

ThingsDB can be extended with modules.

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report for ThingsDB. Following these guidelines helps maintainers and the community understand your report :pencil:, reproduce the behavior :computer: :computer:, and find related reports :mag_right:.

Before creating bug reports, please check [this list](#before-submitting-a-bug-report) as you might find out that you don't need to create one. When you are creating a bug report, please [include as many details as possible](#how-do-i-submit-a-good-bug-report).

> **Note:** If you find a **Closed** issue that seems like it is the same thing that you're experiencing, open a new issue and include a link to the original issue in the body of your new one.

#### How Do I Submit A (Good) Bug Report?

Bugs are tracked as [GitHub issues](https://guides.github.com/features/issues/). After you've determined [which repository](#thingsdb-and-related-projects) your bug is related to, create an issue on that repository.
Explain the problem and include additional details to help maintainers reproduce the problem:

* **Use a clear and descriptive title** for the issue to identify the problem.
* **Describe the exact steps which reproduce the problem** in as many details as possible. For example, start by explaining how you configured yout ThingsDB node(s).
* **Explain which behavior you expected to see instead and why.**
* **If the problem is related to a crash or memory issue**, install [valgrind](https://www.valgrind.org) and run ThingsDB using `valgrind --tool=memcheck thingsdb`.

Provide more context by answering these questions:

* **Can you reproduce the problem with a single node?** Usually ThingsDB is running on multiple nodes, did you try if the issue exists when running only a single node?
* **Did the problem start happening recently** (e.g. after updating to a new version of ThingsDB) or was this always a problem?
* If the problem started happening recently, **can you reproduce the problem in an older version of ThingsDB?** What's the most recent version in which the problem doesn't happen? You can download older versions of ThingsDB from [the releases page](https://github.com/thingsdb/ThingsDB/releases).
* **Can you reliably reproduce the issue?** If not, provide details about how often the problem happens and under which conditions it normally happens.

Include details about your configuration and environment:

* **Which version of ThingsDB are you using?** You can get the exact version by running `thingsdb -v` in your terminal, or by starting ThingsDB and run the query `node_info().load().version;` in the `@node` scope.
* **What's the name and version of the OS you're using**?
* **Which [modules](#thingsdb-and-modules) do you have installed?** You can get that list of module files by running `modules_info().map(|m| m.load().file);` in the `@node` scope.
* **What configuration are you using?** Provide the onfig file and environment variable and optionally provide the output of `node_info();` from the `@node` scope.

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for ThingsDB, including completely new features and minor improvements to existing functionality. Following these guidelines helps maintainers and the community understand your suggestion :pencil: and find related suggestions :mag_right:.

Before creating enhancement suggestions, please check [this list](#before-submitting-an-enhancement-suggestion) as you might find out that you don't need to create one. When you are creating an enhancement suggestion, please [include as many details as possible](#how-do-i-submit-a-good-enhancement-suggestion). Fill in [the template](https://github.com/thingsdb/.github/blob/master/.github/ISSUE_TEMPLATE/feature_request.md), including the steps that you imagine you would take if the feature you're requesting existed.

#### Before Submitting An Enhancement Suggestion

* **Check the [ThingsDB Documentation](https://https://docs.thingsdb.net/)** for tips — you might discover that the enhancement is already available. Most importantly, check if you're using [the latest version of ThingsDB](github.com/thingsdb/ThingsDB/releases/latest).
* **Determine [which repository the enhancement should be suggested in](#thingsdb-and-related-projects).**
* **Perform a [cursory search](https://github.com/search?q=+is%3Aissue+user%3AThingsDB)** to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.

#### How Do I Submit A (Good) Enhancement Suggestion?

Enhancement suggestions are tracked as [GitHub issues](https://guides.github.com/features/issues/). After you've determined [which repository](#thingsdb-and-related-projects) your enhancement suggestion is related to, create an issue on that repository and provide the following information:

* **Use a clear and descriptive title** for the issue to identify the suggestion.
* **Provide a step-by-step description of the suggested enhancement** in as many details as possible.
* **Explain why this enhancement would be useful** to most ThingsDB users and isn't something that can or should be implemented as a [community module](#thingsdb-and-modules).
* **Specify which version of ThingsDB you're using.** You can get the exact version by running `thingsdb -v` in your terminal, or by starting ThingsDB and run the query `node_info().load().version;` in the `@node` scope.
* **Specify the name and version of the OS you're using.**
* **Do you want to suggest a new function for ThingsDB?** 
  * Functions should be short and do exactly one thing.
  * Function names are lower case and if composed of two words an underscore should be user to combine the words. For example `find_index()`.
  * If a function modifies an object, the function should extend the type of the object. For example, `push()` extends type `list`.
  * If a function modifies an object, the return value should *not* be the instance but rather some other useful value. For example, `push()` returns the new length of the list.
  * Functions may accept arguments but functions should never modify one of the arguments. *(If the argument is an identifier to something else, for example a `Type`, the function is allowed to make changes to that `Type`)*

### Your First Code Contribution

Unsure where to begin contributing to ThingsDB? You can start by looking through these `beginner` and `help-wanted` issues:

* [Beginner issues][beginner] - issues which should only require a few lines of code, and a test or two.
* [Help wanted issues][help-wanted] - issues which should be a bit more involved than `beginner` issues.

Both issue lists are sorted by total number of comments. While not perfect, number of comments is a reasonable proxy for impact a given change will have.

#### Local development

ThingsDB Core and modules can be developed locally. 

Once you've set up your fork of the `thingsdb/ThingsDB` repository, you can clone it to your local machine:

```
$ git clone git@github.com:your-username/ThingsDB.git
```

From there, you can navigate into the directory where you've cloned the ThingsDB source code and install all the required dependencies.

See the [build-from-source](https://docs.thingsdb.net/v0/getting-started/build-from-source/) documentation on how to compile ThingsDB.

> **Note:** The documentation above let's you compile the `Release` build. For local development you want to use the `Debug` build instead. The Debug version builds a bit faster, enables all assertions and includes debug symbols.


## Styleguides

### C Styleguide

* All variables must be declared at the top of the relevant function (with one exception). The only exception to this rule is index variables in loops, which may be declared locally.
* Comments must start with `/*` and end with `*/`.
* Braces `{` and `}` go on a new line.
* Indentation is done by using 4 spaces.
* Try to limit the number of characters of a single line to 80.

### Python Styleguide

* [PEP 8](https://www.python.org/dev/peps/pep-0008/) is used as the Style Guide for Python Code.

### Documentation

* Use [ThingsDocs](https://github.com/thingsdb/ThingsDocs).

[beginner]:https://github.com/search?utf8=%E2%9C%93&q=is%3Aopen+is%3Aissue+label%3Abeginner+label%3Ahelp-wanted+user%3AThingsDB+sort%3Acomments-desc
[help-wanted]:https://github.com/search?q=is%3Aopen+is%3Aissue+label%3Ahelp-wanted+user%3AThingsDB+sort%3Acomments-desc+-label%3Abeginner
