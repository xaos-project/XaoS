from docutils import nodes

def setup(app):
    app.add_generic_role('tutor', nodes.emphasis)
