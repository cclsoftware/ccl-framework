namespace TestNamespace {

// Covers a #define preceded by a block comment on the same line (e.g. "/* XSTRING */#define"):
// the comment counts as whitespace, so the define must still resolve the persistent name below.

	/* XSTRING */#define kCommentedPersistentName "CommentedPersistentClass"

class CommentedDefineClass: public Object
{
public:
	DECLARE_CLASS (CommentedDefineClass, Object)
};

DEFINE_CLASS_PERSISTENT (CommentedDefineClass, Object, kCommentedPersistentName)

} // namespace TestNamespace
