def _wrap_json_properties_impl(ctx):
  content = ctx.read(ctx.attr.src)
  bzl = "PROPERTIES = %s" % (content)
  ctx.file("BUILD", content = "", executable = False)
  ctx.file("properties.bzl", content = bzl, executable = False)

wrap_json_properties = repository_rule(
  attrs = {
    "src": attr.label(mandatory = True),
  },
  implementation = _wrap_json_properties_impl,
)
