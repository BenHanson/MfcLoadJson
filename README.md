# MfcLoadJson

MfcLoadJson allows you to:
- Load JSON into an MFC `CTreeCtrl`
- Edit the values in-place in the control
- Export the control back out to a `.json` file

## Building

- Make sure you have downloaded <a href="https://boost.org">boost</a>
- Build `boost::json` if you haven't already
  - `.\b2 --with-json`
- Set environment variable BOOST_ROOT to point to your boost path
- Load the `.slnx` file into Visual Studio

# Example

```json
{
  "name": "example-project",
  "version": "1.0.0",
  "settings": {
    "debug": true,
    "parallel_jobs": 8
  },
  "dependencies": [
    "boost-system",
    "boost-filesystem"
  ]
}
```

is shown as:

![alt text](./png/MfcLoadJson.png "Screenshot")
