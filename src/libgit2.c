
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

// Wrapper function to handle the options structures outside of stanza
// Performs a fetch for the repo 'repo' from the remote 'remote_name' using the
// refspec 'refspec' and depth 'depth'
// 'refspec' may be NULL, in which case the base refspecs will be used
// Requires that the library has been initialized with git_libgit2_init()
int stz_libgit2_fetch(git_repository *repo, const char *remote_name, const char *refspec, int depth)
{
  int err = 0;
	git_remote *remote = NULL;
  git_strarray refspecs = {};
  git_strarray *refspecs_ptr = NULL;
	git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

	/* Prepare options */
  fetch_opts.depth = depth;
  if (refspec) {
    refspecs.strings = &refspec; // safe for one-element pointer array
    refspecs.count = 1;
    refspecs_ptr = &refspecs;
  }

  /* Lookup remote */
  err = git_remote_lookup(&remote, repo, remote_name);
  if (err != GIT_OK) {
    return err;
  }

	/* Perform the fetch */
	err = git_remote_fetch(remote, refspecs_ptr, &fetch_opts, NULL);
  git_remote_free(remote);
  return err;
}

// Wrapper function to handle the options structures outside of stanza
// Performs a checkout of the reference 'refish' for the repo 'repo'
// Requires that the library has been initialized with git_libgit2_init()
int stz_libgit2_checkout(git_repository *repo, const char *refish, git_checkout_strategy_t checkout_strategy)
{
	int err = 0;
  git_object *target_obj = NULL;
  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

  /* Set up options */
  checkout_opts.checkout_strategy = checkout_strategy;

  /* Convert the ref-like string into a git object */
	err = git_revparse_single(&target_obj, repo, refish);
  if (err != GIT_OK) {
    return err;
  }

  /* Update the working tree and index */
	err = git_checkout_tree(repo, target_obj, &checkout_opts);
	if (err != GIT_OK) {
    git_object_free(target_obj);
    return err;
	}

	/* Update head */
  err = git_repository_set_head(repo, refish);
  git_object_free(target_obj);
  return err;
}
