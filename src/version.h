#ifndef VERSION_H
#define VERSION_H

extern const char *GIT_TAG;
extern const char *GIT_REV;
extern const char *GIT_DIRTY;
extern const char *GIT_BRANCH;
extern const char *BUILDDATE;

static inline const char *get_project_name(void) { return PROJECT_NAME; }

static inline const char *get_git_branch(void) { return GIT_BRANCH; }

static inline const char *get_builddate(void) { return BUILDDATE; }

static inline const char *get_git_revision(void) { return GIT_REV; }

static inline const char *get_git_version(void) { if (*GIT_TAG) { return GIT_TAG; } else { return GIT_DIRTY; } }

static inline const char *get_git_version2(void) { return GIT_TAG; }

#endif /* VERSION_H */
