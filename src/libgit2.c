
#include <stdlib.h>
#include <string.h>
#include <git2.h>

/**
  * Helper: Allocate and copy the string
  * Output string must be freed by caller
  */
char *alloc_copy_str (const char *str) {
  size_t len = strlen(str);
  char *out = calloc(len + 1, sizeof(char));
  if (!out) {
    return NULL;
  }
  memcpy(out, str, len + 1);
  return out;
}

/**
  * Helper: Allocate and format the object ID into a hex representation string
  * Output string must be freed by caller if successful
  */
int alloc_format_git_oid (char **out, const git_oid* id) {
  *out = calloc(GIT_OID_MAX_HEXSIZE + 1, sizeof(char));
  int err = git_oid_fmt(*out, id);
  if (err != GIT_OK) {
    free(*out);
    return err;
  }
  (*out)[GIT_OID_MAX_HEXSIZE] = '\0';
  return err;
}

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
	git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

	/* Prepare options */
  fetch_opts.depth = depth;

  /* Lookup remote */
  err = git_remote_lookup(&remote, repo, remote_name);
  if (err != GIT_OK) {
    return err;
  }

	/* Perform the fetch with refspecs if supplied */
  if (refspec) {
    // Make a copy of 'refspec' to satisfy non-const requirement
    size_t refspec_size = strlen(refspec) + 1;
    char *refspec_copy[1] = {0};
    refspec_copy[0] = alloc_copy_str(refspec);

    const git_strarray refspecs = {refspec_copy, 1};
	  err = git_remote_fetch(remote, &refspecs, &fetch_opts, NULL);

    free(refspec_copy[0]);
  }
  else {
	  err = git_remote_fetch(remote, NULL, &fetch_opts, NULL);
  }

  /* Cleanup */
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

// Wrapper function to handle the data structures outside of stanza
// Convert the reference 'refish' into a git_oid
// Requires that the library has been initialized with git_libgit2_init()
// Caller must free the output string
int stz_libgit2_revparse(char **out, git_repository *repo, const char *refish)
{
	int err = 0;
  git_object *target_obj = NULL;

  /* Convert the ref-like string into a git object */
	err = git_revparse_single(&target_obj, repo, refish);
  if (err != GIT_OK) {
    return err;
  }

  /* Format the object's ID into a hex string */
  err = alloc_format_git_oid(out, git_object_id(target_obj));
  if (err != GIT_OK) {
    git_object_free(target_obj);
    return err;
  }

  /* Cleanup */
  git_object_free(target_obj);

  return GIT_OK;
}

// Helper for stz_libgit2_lsremote and stz_libgit2_lsremote_url
// Connects to the remote and allocates strings for all the remote's references
// and their ID hashes
// Caller must free all strings in the output arrays and the arrays themselves
int alloc_list_refs(char ***ids_out, char ***names_out, long long *out_len, git_remote *remote)
{
  int err;
  size_t i, j;

  git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
  const git_remote_head **refs;
  size_t refs_len;
  char **ref_ids = NULL;
  char **ref_names = NULL;

  /* Connect to remote */
  err = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, NULL, NULL);
  if (err != GIT_OK) {
    return err;
  }

  /* List remote references */
  err = git_remote_ls(&refs, &refs_len, remote);
  if (err != GIT_OK) {
    return err;
  }

  /* Format references */
  ref_ids = calloc(refs_len, sizeof(char *));
  ref_names = calloc(refs_len, sizeof(char *));
  for (i = 0; i < refs_len; i++) {
    /* Format reference ID */
    err = alloc_format_git_oid(&(ref_ids[i]), &(refs[i]->oid));
    if (err != GIT_OK) {
      for (j = 0; j < i; j++) {
        free(ref_ids[j]);
        free(ref_names[j]);
      }
      free(ref_ids);
      free(ref_names);
      return err;
    }

    /* Copy reference name */
    ref_names[i] = alloc_copy_str(refs[i]->name);
  }

  /* Copy to output */
  *ids_out = ref_ids;
  *names_out = ref_names;
  *out_len = refs_len;

  return GIT_OK;
}

// Wrapper function to handle the option structures outside of stanza
// List all available references of the remote 'remote_name' of the repository
// 'repo'
// 'ids_out' will contain the hex-formatted object IDs for each reference
// 'names_out' will contain the names of each reference
// Requires that the library has been initialized with git_libgit2_init()
// Caller must free all strings in the output arrays and the arrays themselves
int stz_libgit2_lsremote(char ***ids_out, char ***names_out, long long *out_len,
                         git_repository *repo, const char *remote_name)
{
  int err;
  git_remote *remote = NULL;

  /* Lookup remote */
  err = git_remote_lookup(&remote, repo, remote_name);
  if (err != GIT_OK) {
    return err;
  }

  /* List remote references */
  err = alloc_list_refs(ids_out, names_out, out_len, remote);
  if (err != GIT_OK) {
    git_remote_free(remote);
    return err;
  }

  /* Cleanup */
  git_remote_free(remote);

  return GIT_OK;
}

// Wrapper function to handle the option structures outside of stanza
// List all available references of the remote at the URL 'remote_url'
// 'ids_out' will contain the hex-formatted object IDs for each reference
// 'names_out' will contain the names of each reference
// Requires that the library has been initialized with git_libgit2_init()
// Caller must free all strings in the output arrays and the arrays themselves
int stz_libgit2_lsremote_url(char ***ids_out, char ***names_out,
                             long long *out_len, const char *remote_url)
{
  int err;
  git_remote *remote = NULL;
	git_remote_create_options remote_create_opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

  /* Lookup remote */
  err = git_remote_create_with_opts(&remote, remote_url, &remote_create_opts);
  if (err != GIT_OK) {
    return err;
  }

  /* List remote references */
  err = alloc_list_refs(ids_out, names_out, out_len, remote);
  if (err != GIT_OK) {
    git_remote_free(remote);
    return err;
  }

  /* Cleanup */
  git_remote_free(remote);

  return GIT_OK;
}
