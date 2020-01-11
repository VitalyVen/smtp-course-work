# # smtp-course-work bmstu 2020
Environment requirements:
+ OS: Ubuntu 18.04 x64
+ Python: 3.7.5
+ Evolution ([install](https://rc.partners.org/kb/article/2702) | [eMail setup](https://askubuntu.com/questions/51467/how-do-i-setup-an-email-account-in-evolution))

### How to run
```sh
    python3 main.py
```

## Maildir guide
Currently, maildir is a directory that stores email messages as files. Maildir works with Courier, a mail server that provides folders and quotas for the email accounts on your hosting account.

The folders inside the mail directory are subdirectories such as *.Drafts*, *.Trash* and *.Sent*. Each of these folders contain three additional subdirectories called *tmp*, *new* and *cur*.

+ **tmp** - This subdirectory stores email messages that are in the process of being delivered. It may also store other kinds of temporary files.
+ **new** - This subdirectory stores messages that have been delivered but have not yet been seen by any mail application, such as webmail or Outlook.
+ **cur** - This subdirectory stores messages that have already been viewed by mail applications, like webmail or Outlook.

## About [Content-Transfer-Encoding](https://github.com/VitalyVen/smtp-course-work/issues/6)
CODE TRANSFER NOTE: The quoted-printable and base64 converters are designed so that the data after its use is easily interconvertible. The only nuance that arises in such a relay is a sign of the end of the line. When converting from quoted-printable to base64, the newline should be replaced with the CRLF sequence. Accordingly, and vice versa, but ONLY when converting text data.

## Tests 
```sh
    export PYTHONPATH=`pwd`
    cd server && pytest-3
```

### Tests with coverage
    coverage run -m pytest --cov-report=html --cov=.
# References
1. J. Klensin, Network Working Group (October 2008) ([DRAFT STANDARD](https://tools.ietf.org/html/rfc5321))
2. SMTP protocol [Explained](https://www.afternerd.com/blog/smtp/) (How Email works?)
