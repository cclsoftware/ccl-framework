{{ file.anchor|anchor -}}
{{ (file.name + ' (' + file.kind_string + ')')|heading1(false) -}}
{% if file.breadcrumb %}
{{ (file.breadcrumb|map('link_ref')|join(' > ') + ' ')|paragraph -}}
{% endif %}
