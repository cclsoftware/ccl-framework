{% import '_macros.rst' as m %}
{{ root_anchor|anchor -}}
{% include 'group_summary_file.rst' %}
{{ 'Index'|heading2 -}}
{{ m.toctree(toc_files) -}}
