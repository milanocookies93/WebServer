WebServer written in C that can deploy a static website from a git repository.

Originally a CS 241 MP, but it is now modified to be memory efficient and work with github.

You can run the server by compiling and executing the following:

`
    ./server [URL to repository] &;disown;
`

If you set a webhook via the [Github API](https://developer.github.com/webhooks/), your site will update for each push. Otherwise you can run the deploy.sh script.

To Do
* Allow customized POST requests
* Set up link redirects

Since this is an outdated MP and UIUC is not attempting to remove repos I am assuming it is acceptable to make this public. Upon request by the University I am happy to take this down. However I would also suggest you get [these](https://www.google.com/?gws_rd=ssl#q=cs+225+github) and [these](https://www.google.com/?gws_rd=ssl#q=cs+241+github) taken down as well.
