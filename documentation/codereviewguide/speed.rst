#####################
Speed of code reviews
#####################

**The speed at which the team can develop a product is more important than the speed at which an individual developer can write code.**

When code reviews are slow, several things happen:

- The velocity of the team is decreased
- Developers start to protest the code review process
- Code health can decrease - pressure to accept code due to long iterations

===================================
How long should a code review take?
===================================

When talking about the speed of code reviews, we are mostly concerned with the response time as opposed to the time it takes for the changeset to get through the whole review and be submitted. Ideally, both should be fast but a timely reponse is more important.

- If you are in the middle of a focused task like coding, don’t interrupt yourself with a code review
- Wait for a break point in your work, e.g. when a task is completed, after a meeting, after lunch or after taking a break to do a code review
- If you are not in the middle of a focused task you should do a code review shortly after it comes in
- If you are too busy to do a full review on a changeset when it comes in, you can still send a quick response that lets the developer know when you will get to it, suggest other reviewers who might be able to respond more quickly, or provide some initial broad comment

============================
How to speed up code reviews
============================

- Keep changes to the codebase small
- If a change is too large to be reviewed presently, ask authors to split into smaller changes which build upon each other
- If requested changes to the code are small and the reviewer is confident that the author will implement them correctly, the code can be approved with comments
- If architectural changes are required or the author asks for broad feedback on the general design, avoid reviewing details like style inconsistencies