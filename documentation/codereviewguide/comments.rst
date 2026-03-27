#################################
How to write code review comments
#################################

=======
Summary
=======
- Be kind
- Explain your reasoning
- Balance giving explicit directions with just pointing out problems and letting the developer decide
- Encourage developers to simplify code or add code comments instead of just explaining the complexity to you

===========
Explain Why
===========

For developers to learn and to require less review in the future it is essential that they understand why each change in a review needs to happen.

===================
Communicate Clearly
===================

- Avoid ambiguity in comments
- When requesting changes, clearly define what you want to have changed and why
- Use inline comments where feasible

========================
Respectful communication
========================

Always be friendly, professional and unemotional. Talk about the code, not the person and provide helpful advice in order to help authors improve their skills so they require less review in the future.

-------
Example
-------

Bad: “Why did you use threads here when there’s obviously no benefit to be gained from concurrency?”

Good: “The concurrency model here is adding complexity to the system without any actual performance benefit that I can see. Because there’s no performance benefit, it’s best for this code to be single-threaded instead of using multiple threads.”

==========
Guidelines
==========

- Assume competence & goodwill
  - We attract competent people - and that means even when they‘re wrong, it most likely comes from lack of information, not from inability. A “bad” change usually means one of the parties is in possession of information the other one isn’t aware of.
- Explain why
  - It might be obvious to you that some code is wrong, but it‘s probably not obvious to the author — or they wouldn’t have written it that way. So please don‘t say “This is wrong”. Instead, explain at least what the right way looks like. Or even better, explain why they should do things differently.
- Don’t shame people
  - “How could you not see this” is a very unhelpful thing to say. Assume that your colleagues do their best, but occasionally make mistakes. That's why we have code reviews - to spot those mistakes.
- Don’t use extreme or negative language
  - Please don‘t say things like “no sane person would ever do this” or “this algorithm is terrible”, whether it’s about the change you‘re reviewing or about the surrounding code. While it might intimidate the reviewee into doing what you want, it’s not helpful in the long run - they will feel incapable, and there is not much info in there to help them improve. Discuss the code, not the person.
- Don’t bikeshed (see `bikeshedding <https://en.wikipedia.org/wiki/Bikeshedding>`_)
  - Always ask yourself if this decision really matters in the long run, or if you‘re enforcing a subjective preference. It feels good to be right, but only one of the two participants can win that game. If it’s not important, agree to disagree, and move on.