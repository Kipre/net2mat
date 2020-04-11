## Call syntax

`net2mat` can be called with 3 different syntaxes:

- `> net2mat /path/to/file.net` in this case the `.mat` file is created in the same directory and has the same filename and only the extension changes: `/path/to/file.mat`.
- `> net2mat /path/to/file.net /path/to/output/` in this case the `.mat` file is created in the output directory with the same filename: `/path/to/output/file.mat`.
- `> net2mat /path/to/file.net /path/to/output/output_file.mat` in this case the `.mat` file is created in the output path as expected.

## Examples 

```
C:\Users\kipr\Documents\GitHub\net2mat>net2mat test_data/GasLib-11.net example1.mat
Opened test_data/GasLib-11.net with success
Fetched data for nodes, found 11 nodes
Fetched data for connections, found 11 connections
Made incidence matrix and transformed connections data
Transformed nodes data
Created the example1.mat file
Wrote everithing
Done

C:\Users\kipr\Documents\GitHub\net2mat>net2mat C:\Users\kipr\Documents\GitHub\net2mat\test_data\GasLib-11.net C:\Users\kipr\Downloads
Opened C:\Users\kipr\Documents\GitHub\net2mat\test_data\GasLib-11.net with success
Fetched data for nodes, found 11 nodes
Fetched data for connections, found 11 connections
Made incidence matrix and transformed connections data
Transformed nodes data
Created the C:\Users\kipr\Downloads\GasLib-11.mat file
Wrote everithing
Done
```