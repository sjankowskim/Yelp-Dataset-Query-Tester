# Yelp Dataset Query Tester

This program is designed for testing queries on the [Yelp Dataset](https://www.yelp.com/dataset). It is based off Sam's code for the IMDb dataset.

## Prerequisites

Unlike the IMDb dataset, this data does not come pre-made with .tsv files that we want. I had to make a separate parser to extract and form the data in the way we want.

The first step then before running anything is to obtain the `yelp_dataset`. Download the zip folder, place it in `/data`, and extract it. You should now have `/data/yelp_dataset` and in that folder files named `yelp_academic_dataset_???.json`.

Next, run `parser.py` using Python. This should create four new files in the `/data` folder (NOT `/data/yelp_dataset`): `yelp_academic_dataset_review.tsv`, `yelp_academic_dataset_business.tsv`, `yelp_academic_dataset_review.tsv`, and `yelp_academic_dataset_tip.txv`. 
Note that it might give you an error saying "invalid control character at...". I have wrapped this in a try-except block and simply ignore the lines that cause this issue.

Unfortunately, we are still not yet done. In yelp's infinite wisdom, they added duplicate data to their dataset which was not handled by the python script. This would be problematic once we want to create the databse in SQLite 3. We can fix this using some UNIX commands. Traverse into the `/data` directory and paste the following into the console...
`head -n 1 data/yelp_academic_dataset_tip.tsv > temp_file.tsv && tail -n +2 data/yelp_academic_dataset_tip.tsv | sort -t$'\t' -k2,2 -k4,4 -k5,5 | awk -F'\t' '!seen[$2 FS $4 FS $5]++' >> temp_file.tsv && mv temp_file.tsv data/yelp_academic_dataset_tip.tsv && sed -i '2d' data/yelp_academic_dataset_tip.tsv`

Before running the program, ensure that the `/data` directory is populated with all the appropriate TSV files:

```
.
├── yelp_academic_dataset_business.tsv
├── yelp_academic_dataset_review.tsv
├── yelp_academic_dataset_tip.tsv
└── yelp_academic_dataset_user.tsv
```

## Compilation

To compile the program, use the following command:

```
make
```

## Execution

Run the program using the command:

```
./testQuery
```

### Instructions

Upon launching the executable, you will be prompted to type `'y'` to execute the stored query, or `'n'` to exit the program. Typing `'y'` will execute the current query stored in `query.txt`. 

- The results of the query will be saved in `result.txt`.
- Timing information will be displayed on the standard output.

## Performance Considerations

- The IMDB Non-Commercial Dataset is quite large, so loading it initially can take a few minutes.
- If you've already loaded the dataset and would like to re-launch the program without waiting for it to reload, you can use the `--preserve` parameter as shown below:

  ```
  ./testQuery --preserve
  ```

  This will bypass reloading the dataset and instead use the `moviedb.sqlite` that already exists. 

*Important:* If you haven't generated this for the first time or have overwritten it with incomplete data, you should not use the `--preserve` parameter.