##################################
What to look for in a code review?
##################################

==========
Principles
==========

- Technical facts and data overrule opinions and personal preferences
- On matters of style, the style guide is the absolute authority. Any purely style point that is not in the style guide is a matter of personal preference
  - The style should be consistent with what is there (function, file, module)
  - If there is no previous style, accept the author’s

======
Design
======

- Do the interactions of code within the change make sense?
- Is the code in the correct place, should it be moved or extracted?
- Does it integrate with the rest of the system?
- Is it a good time for the change to be applied?

=============
Functionality
=============

- Does the change accomplish the intended purpose?
- Does it add value to the users of the code (Both developers and end users)?
- Check common issues

==========
Complexity
==========

Complex means “Can’t be understood quickly by readers of the code” or “Bugs are likely to be introduced by other developers working with the code”.
- Evaluate complexity at every level: line, function, class, …

=====
Tests
=====

- Does the code contain changes which should be tested?
- Do the tests have good names?
- Would the tests fail if the code didn’t do the right thing?

=================
Naming & Comments
=================

- Did the author pick good names for everything?
  - A good name is long enough to fully communicate what the item is or does, without being so long that it becomes hard to read
- Are the added comments necessary and meaningful?

=============
Documentation
=============

Check if the code requires documentation to be added or changed.

===========
Good Things
===========

If you see something nice in a changeset, tell the author. Encouraging authors and appreciating work is sometimes even more valuable, in terms of mentoring, than to tell them what they did wrong.

=======
Summary
=======

- The code is well-designed
- The functionality is good for the users of the code
- The code isn’t more complex than it needs to be
- The developer isn’t implementing things they might need in the future but don’t know they need now
- The developer used clear names for everything
- Comments are clear and useful, and mostly explain why instead of what
- Code is appropriately documented
- The code conforms to our style guides
- Code is tested where appropriate
- If tests covering the change exist, extend them as necessary and make sure they still pass