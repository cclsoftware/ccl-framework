{% include '_header.rst' %}
{% if file.variable_type_link %}
{{ (file.comment_body + 'Type: ' + (file.variable_type_link|autolink))|paragraph -}}
{% else %}
{{ file.comment_body|paragraph -}}
{% endif %}
