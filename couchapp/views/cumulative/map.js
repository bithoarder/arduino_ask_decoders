function(doc)
{
  if(doc.cumulative && doc.channel==1)
  {
    emit(new Date(doc.time*1000), doc.cumulative/1000.0);
  }
}

