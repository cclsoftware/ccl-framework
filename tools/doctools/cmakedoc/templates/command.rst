{# Print command details: name, comment, parameters. #}
.. py:function:: {{ command.name_with_args }}

	``{{ command.kind }}`` ``{{ command.group|lower }}`` ``{{ command.source }}``

{% if command.comment|length > 0 %}
{% for comment_line in command.comment %}
	{{ comment_line }}
{% endfor %}

{% endif %}
{% if command.params|length > 0 %}
{% for param in command.params -%}
{% if param.comment %}
{% if param.type %}
	:param {{ param.type|lower }} {{ param.name }}: {{ param.comment }}
{% else %}
	:param {{ param.name }}: {{ param.comment }}
{% endif %}
{% endif %}
{% endfor %}

{% endif %}