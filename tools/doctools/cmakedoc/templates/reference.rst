.. _cmake-reference-topofpage:

#########
Reference
#########

******
Groups
******

{% for group in grouplist %}

{{ group.name }} ({{ group.commands|length }})
================================================

{% for command in group.commands %}[:ref:`{{ command.name }} <{{ command.refid }}>`]{{ " " if not loop.last }}{% endfor %}

{% endfor %}


-----

{# Print all commands grouped by kind. #}
{% for kind in kindlist %}

*************************************************
{{ kind.name }} ({{ kind.commands|length }})
*************************************************

{% for command in kind.commands %}
.. _{{ command.refid }}:

{# Expect command.rst to add empty line to separate from back to top link #}
{% include 'command.rst' %}
> :ref:`back to top <cmake-reference-topofpage>`


{% endfor %}
{% endfor %}

