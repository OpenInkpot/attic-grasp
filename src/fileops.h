/* check if directory exists*/
static __inline__ int check_dir(char *path)
{
	int r;

	if (access(path, R_OK | W_OK | X_OK))
		r = errno;
	else
		r = 0;

	if (r == EACCES || r == EPERM) {
		/* it isn't accessible */
		DBG("Path %s already exists and is inaccessible\n", path);

		return GE_ACCESS;
	} else if (r == EROFS || r == EFAULT) {
		/* or otherwise broken */
		DBG("Path %s is broken.\n", path);

		return GE_ACCESS;
	} else if (r == ENOENT) {
		/* it doesn't exist */
		DBG("# path %s doesn't exist\n", path);

		return GE_NOENT;
	} else if (r == 0) {
		/* it does exist */
		DBG("# path %s does exist\n", path);

		return GE_OK;
	}

	DBG("# unexpected error %d while trying to resolve %s\n",
			r, path);

	return GE_ERROR;
}

static __inline__ int check_file(char *path)
{
	int r;

	if (access(path, R_OK | W_OK))
		r = errno;
	else
		r = 0;

	if (r == EACCES || r == EPERM) {
		/* it isn't accessible */
		DBG("Path %s already exists and is inaccessible\n", path);

		return GE_ACCESS;
	} else if (r == EROFS || r == EFAULT) {
		/* or otherwise broken */
		DBG("Path %s is broken.\n", path);

		return GE_ACCESS;
	} else if (r == ENOENT) {
		/* it doesn't exist */
		DBG("# path %s doesn't exist\n", path);

		return GE_NOENT;
	} else if (r == 0) {
		/* it does exist */
		DBG("# path %s does exist\n", path);

		return GE_OK;
	}

	DBG("# unexpected error %d while trying to resolve %s\n",
			r, path);

	return GE_ERROR;
}

static __inline__ int check_and_create_dir(char *path)
{
	int r = check_dir(path);

	if (r == GE_NOENT) {
		if (mkdir(path, 0755)) {
			DBG("# failed to create %s\n", path);
			return GE_ERROR;
		}

		return GE_OK;
	}

	return r;
}

