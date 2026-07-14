# mdl rules https://github.com/markdownlint/markdownlint/blob/master/docs/RULES.md

# Import all default rules
all

# Only allow atx style headings (e.g. # H1 ## H2)
rule 'MD003', :style => :atx

# Only allow dashes in unordered lists
rule 'MD004', :style => :dash

# Do not enforce line length on code blocks
rule 'MD013', :code_blocks => false

# Lists should be numbered sequentially in text
rule 'MD029', :style => :ordered

# Ignore blockquotes separated only be a blank line. This is a limitation of
# some markdown parsers, not markdown itself.
exclude_rule 'MD028'

# Allow non-top-level headings as first heading,
# since the layout will put an extra heading there
exclude_rule 'MD002'
exclude_rule 'MD041'
