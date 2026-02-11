"""Comment format definitions and util class."""

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class CommentTemplates:
    """
    Format definitions for generating a TypeDoc conform comment block.
    """

    #
    # /**
    #  %s (text lines here, ending with linebreak each)
    #  */
    #
    BLOCK = "/**\n%s */\n"

    # row with indentation and linebreak
    ROW = " * %s\n"

    # method brief section
    CLASS_BRIEF = ROW % "Script class '%s'."

    # method brief section
    METHOD_BRIEF = ROW % "Class script method '%s()'."

    # @param, w/o default value (optional)
    PARAM = ROW % "@param %s  type: %s"
    PARAM_OPTIONAL = ROW % "@param %s  type: %s (optional, default: %s)"

    # @returns
    RETURNS = ROW % "@returns %s"


class CommentStringBuilder:
    """ Comment string builder. """

    def __init__(self):
        self.params_str = ""
        self.brief_str = ""
        self.returns_str = ""

    def add_class_brief(self, class_name: str) -> None:
        if not self.brief_str:
            self.brief_str = CommentTemplates.CLASS_BRIEF % class_name

    def add_method_brief(self, method_name: str) -> None:
        """ Add method brief description based on method name. """
        if not self.brief_str:
            self.brief_str = CommentTemplates.METHOD_BRIEF % method_name

    def add_param(self, name: str, type_string: str, default_value: str) -> None:
        """ Add param description, covering default values if available. """
        if default_value:
            self.params_str += CommentTemplates.PARAM_OPTIONAL % (name, type_string, default_value)
        else:
            self.params_str += CommentTemplates.PARAM % (name, type_string)

    def add_returns(self, type_string: str) -> None:
        """ Add return type. """
        if not self.returns_str:
            self.returns_str = CommentTemplates.RETURNS % type_string

    def to_string(self) -> str:
        """ Convert to string, ends with new line. """

        # separate brief and params if needed
        separation = ""
        if self.params_str:
            separation = CommentTemplates.ROW % ""

        content = self.brief_str + separation + self.params_str + self.returns_str
        return CommentTemplates.BLOCK % content
