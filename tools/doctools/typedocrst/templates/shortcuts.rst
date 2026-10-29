{% for row in rows %}
{% if row.heading_comment %}
{{ row.heading_comment|comment_line -}}
{% endif %}
{{ row.alias|replace_line(row.value) }}
{% endfor %}
