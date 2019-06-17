
for i in test_qsort*; do
  echo $i
  if [ -x "$i" ]; then
    echo "Running $i: `date`"
    perf stat -d -d -d --sync --field-separator=, --repeat 15 -o results_$i.csv ./$i 8000
    echo "Completed $i: `date`"
    date
  fi
done
