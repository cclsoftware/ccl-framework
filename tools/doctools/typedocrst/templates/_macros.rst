{% macro render_group_details(group) %}
{{ group.title|heading2 -}}
{% for child in group.children %}
{{ child -}}
{% if not loop.last %}
{{ hr() -}}
{% endif %}
{% endfor %}
{% endmacro %}

{% macro render_group_summary(group) %}{{ bold(group.title) + ': ' + (group.links|map('autolink')|join('')) }}{% endmacro %}

{% macro render_summary_section(groups) %}
{% filter paragraph %}
{{ 'Summary'|heading2 -}}
{% for g in groups %}{{ render_group_summary(g) }}{% endfor %}
{% endfilter %}
{% endmacro %}

{% macro toctree(files) %}
.. toctree::
	:maxdepth: 1

{% for f in files %}
	{{ f }}
{% endfor %}

{% endmacro %}
