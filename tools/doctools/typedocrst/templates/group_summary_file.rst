{# 'group_summary' shape: Project, Namespace, Type Alias -- members get their own file, so only link to them #}
{% import '_macros.rst' as m %}
{% include '_header.rst' %}
{% if file.groups %}
{{ file.comment_body -}}
{{ m.render_summary_section(file.groups) -}}
{% endif %}
