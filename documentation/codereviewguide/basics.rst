######
Basics
######

===========================
The purpose of code reviews
===========================

From Google’s review guidelines:

- “The primary purpose of code review is to make sure that the overall code health of the code base is improving over time. All of the tools and processes of code review are designed to this end.”
- “A secondary goal is improving the skills of developers so that they require less and less review over time.”

=========================
When to do a code review?
=========================

When talking about code reviews, we typically refer to the review of changes applied to our codebase. Every non-trivial change should be reviewed by at least one person. If you are not absolutely sure that your change is a working improvement to the codebase, have it reviewed.

Trivial changes don’t need to be reviewed.

========================
How to choose reviewers?
========================

- Try to choose reviewers closest to the code and with the required expertise to evaluate the changes
- Add experienced reviewers and unexperienced reviewers
- Don’t add too many reviewers in order to keep reviews efficient

Important: **Reviewers take ownership and responsibility for the code they are reviewing.**

==============================
When to approve a code review?
==============================

In general, reviewers should favor approving a change once it is in a state where it definitely improves the overall code health of the system being worked on, even if the change isn’t perfect.

===================
Resolving Conflicts
===================

A face-to-face meeting can help with coming to a consensus between author and reviewer
- Document the result within the review