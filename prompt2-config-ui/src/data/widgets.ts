export interface WidgetDef {
  token: string;
  description: string;
  group: 'SYS' | 'Repo' | 'AWS' | 'Other';
  /** Whether this widget only makes sense in PROMPT.GIT */
  gitOnly: boolean;
}

export const widgets: WidgetDef[] = [
  // System
  { token: 'SYS.username',    description: 'Username',                       group: 'SYS',   gitOnly: false },
  { token: 'SYS.hostname',    description: 'Hostname',                       group: 'SYS',   gitOnly: false },
  { token: 'SYS.uid',         description: 'Effective UID',                  group: 'SYS',   gitOnly: false },
  { token: 'SYS.gid',         description: 'Effective GID',                  group: 'SYS',   gitOnly: false },
  { token: 'SYS.promptchar',  description: '# if root, $ otherwise',        group: 'SYS',   gitOnly: false },

  // Repository
  { token: 'Repo.is_git_repo',    description: 'Whether . is a git repo',       group: 'Repo', gitOnly: true },
  { token: 'Repo.is_nascent_repo', description: 'Whether repo has no commits',  group: 'Repo', gitOnly: true },
  { token: 'Repo.name',           description: 'Repository name',               group: 'Repo', gitOnly: true },
  { token: 'Repo.branch_name',    description: 'Current branch name',           group: 'Repo', gitOnly: true },
  { token: 'Repo.rebase_active',  description: 'If interactive rebase is active', group: 'Repo', gitOnly: true },
  { token: 'Repo.conflicts',      description: 'Number of conflicts',           group: 'Repo', gitOnly: true },
  { token: 'Repo.ahead',          description: 'Commits ahead of upstream',     group: 'Repo', gitOnly: true },
  { token: 'Repo.behind',         description: 'Commits behind upstream',       group: 'Repo', gitOnly: true },
  { token: 'Repo.staged',         description: 'Staged files count',            group: 'Repo', gitOnly: true },
  { token: 'Repo.modified',       description: 'Modified files count',          group: 'Repo', gitOnly: true },
  { token: 'Repo.untracked',      description: 'Untracked files count',         group: 'Repo', gitOnly: true },

  // AWS
  { token: 'AWS.token_is_valid',        description: 'Whether AWS SSO token is valid', group: 'AWS', gitOnly: false },
  { token: 'AWS.token_remaining_hours',  description: 'AWS token remaining hours',     group: 'AWS', gitOnly: false },
  { token: 'AWS.token_remaining_minutes', description: 'AWS token remaining minutes',  group: 'AWS', gitOnly: false },

  // Other
  { token: 'CWD', description: 'Current working directory',         group: 'Other', gitOnly: false },
  { token: 'SPC', description: 'Space filler (right-align rest)',    group: 'Other', gitOnly: false },
];
