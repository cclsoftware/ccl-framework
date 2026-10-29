{# 'group_details' shape: Class, Interface, Enum -- members (methods/properties/enum members) get
   no file of their own, so render them here in full, in addition to the summary links #}
{% import '_macros.rst' as m %}
{% include '_header.rst' %}
{% if file.groups %}
{{ file.comment_body -}}
{{ m.render_summary_section(file.groups) -}}
{% for g in file.groups %}
{{ m.render_group_details(g) -}}
{% endfor %}
{% endif %}
