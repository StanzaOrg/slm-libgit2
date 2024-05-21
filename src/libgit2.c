
#include <git2.h>

// Wrapper function to handle the options structures outside of stanza
// Creates a clone of the repo at 'url' in 'local_path' with the given depth
// Set depth = 0 for full (normal) clone
// Assumes library has already been initialized with git_libgit2_init()
// Out param git_repository must be freed with git_repository_free()
// Returns the same return code as git_clone()
int stz_libgit2_clone(git_repository **out, const char *url, const char *local_path, int depth)
{
  git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;

  /* Set up options */
  clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
  clone_opts.fetch_opts.depth = depth;

  /* Do the clone */
  return git_clone(out, url, local_path, &clone_opts);
}
