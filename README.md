# smtp-course-work

## 1. Maildir guide
Currently, maildir is a directory that stores email messages as files. Maildir works with Courier, a mail server that provides folders and quotas for the email accounts on your hosting account.

The folders inside the mail directory are subdirectories such as *.Drafts*, *.Trash* and *.Sent*. Each of these folders contain three additional subdirectories called *tmp*, *new* and *cur*.

+ **tmp** - This subdirectory stores email messages that are in the process of being delivered. It may also store other kinds of temporary files.
+ **new** - This subdirectory stores messages that have been delivered but have not yet been seen by any mail application, such as webmail or Outlook.
+ **cur** - This subdirectory stores messages that have already been viewed by mail applications, like webmail or Outlook.

## About [Content-Transfer-Encoding](https://github.com/VitalyVen/smtp-course-work/issues/6)
CODE TRANSFER NOTE: The quoted-printable and base64 converters are designed so that the data after its use is easily interconvertible. The only nuance that arises in such a relay is a sign of the end of the line. When converting from quoted-printable to base64, the newline should be replaced with the CRLF sequence. Accordingly, and vice versa, but ONLY when converting text data.

## Tests 
    tests can be launched with
    pytest from project root
### Tests with coverage
    coverage run -m pytest --cov-report=html --cov=.
